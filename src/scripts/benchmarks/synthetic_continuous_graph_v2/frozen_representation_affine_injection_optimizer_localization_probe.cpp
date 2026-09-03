// Development-only affine-injection and optimizer-localization evaluator.
//
// The canonical frozen-probe parser and metric vocabulary are included
// directly. The conditioned ridge fit below is the fixed-alpha subset of the
// Phase 2A solver (source SHA-256 pinned by the compile-only wrapper).
#define main cuwacunu_embedded_frozen_representation_affine_probe_main
#include "frozen_representation_affine_probe.cpp"
#undef main

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <random>
#include <unistd.h>

namespace {

constexpr std::string_view kSchema =
    "synthetic_v2_frozen_representation_affine_injection_optimizer_"
    "localization_development_v1";
constexpr int64_t kFeatureWidth = 96;
constexpr int64_t kHiddenWidth = 128;
constexpr int64_t kHeadCount = kEdgeCount * kChannelCount;
constexpr double kFixedRidge = 1.0e-12;
constexpr double kFeatureStandardDeviationFloor = 1.0e-8;
constexpr int64_t kSeed = 31;
constexpr int64_t kSteps = 3500;
constexpr int64_t kBatchSize = 64;
constexpr double kLearningRate = 1.0e-3;
constexpr double kAdamBeta1 = 0.9;
constexpr double kAdamBeta2 = 0.999;
constexpr double kAdamEpsilon = 1.0e-8;
constexpr double kWeightDecay = 0.0;
constexpr double kGradientClipNorm = 5.0;
constexpr double kDirectFloat32ParityTolerance = 1.0e-3;
constexpr double kPairedGeluParityTolerance = 1.0e-5;

struct LocalOptions {
  bool development_only{false};
  std::filesystem::path train_input;
  std::filesystem::path validation_input;
  std::filesystem::path output;
};

struct ConditionedAffineModel {
  torch::Tensor mean;    // [D], float64.
  torch::Tensor inv_std; // [D], float64.
  torch::Tensor weights; // [E,C,D], float64, original target units.
  torch::Tensor bias;    // [E,C], float64, original target units.
  double maximum_normalized_residual{0.0};
  double coefficient_l2_norm{0.0};
};

struct StandardizedFeatures {
  torch::Tensor train;      // [N,D], float32.
  torch::Tensor validation; // [N,D], float32.
  torch::Tensor mean;       // [D], float32.
  torch::Tensor scale;      // [D], float32.
  int64_t clamped_coordinate_count{0};
};

struct StandardizedTarget {
  torch::Tensor train;      // [N], float32.
  torch::Tensor validation; // [N], float32.
  torch::Tensor mean;       // [E,C], float64.
  torch::Tensor scale;      // [E,C], float64.
  int64_t clamped_coordinate_count{0};
};

struct Float32HeadMap {
  torch::Tensor weights; // [H,D], float32, standardized target units.
  torch::Tensor bias;    // [H], float32, standardized target units.
};

struct RouteMetrics {
  MetricSummary aggregate{};
  std::array<MetricSummary, kChannelCount> channels{};
};

struct BatchSchedule {
  torch::Tensor indices; // [steps,batch], int64.
  std::string fingerprint;
};

struct LinearTrainingSummary {
  double initial_full_train_standardized_mse{0.0};
  double final_full_train_standardized_mse{0.0};
  double last_minibatch_loss{0.0};
  double maximum_preclip_gradient_norm{0.0};
  int64_t clipped_step_count{0};
  int64_t optimizer_steps{0};
  std::array<double, kHeadCount> per_head_full_train_standardized_mse{};
};

[[noreturn]] void local_fail(const std::string &message) {
  throw std::runtime_error(message);
}

LocalOptions parse_local_options(int argc, char **argv) {
  LocalOptions options;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    auto value = [&](const char *flag) {
      if (index + 1 >= argc) {
        local_fail(std::string("missing value for ") + flag);
      }
      return std::string(argv[++index]);
    };
    if (argument == "--development-only") {
      if (options.development_only) {
        local_fail("duplicate --development-only");
      }
      options.development_only = true;
    } else if (argument == "--train-input") {
      if (!options.train_input.empty()) {
        local_fail("duplicate --train-input");
      }
      options.train_input = value("--train-input");
    } else if (argument == "--validation-input") {
      if (!options.validation_input.empty()) {
        local_fail("duplicate --validation-input");
      }
      options.validation_input = value("--validation-input");
    } else if (argument == "--output") {
      if (!options.output.empty()) {
        local_fail("duplicate --output");
      }
      options.output = value("--output");
    } else if (argument == "--certified-input" || argument == "--final-input" ||
               argument == "--holdout-input" || argument == "--policy-input" ||
               argument == "--checkpoint" || argument == "--model") {
      local_fail("forbidden protected or model input: " + argument);
    } else {
      local_fail("unknown argument: " + argument);
    }
  }
  if (!options.development_only || options.train_input.empty() ||
      options.validation_input.empty() || options.output.empty()) {
    local_fail("--development-only --train-input --validation-input and "
               "--output are required");
  }
  if (!options.train_input.is_absolute() ||
      !options.validation_input.is_absolute() ||
      !options.output.is_absolute()) {
    local_fail("train, validation, and output paths must be absolute");
  }
  std::set<std::filesystem::path> paths;
  for (const auto &path :
       {options.train_input, options.validation_input, options.output}) {
    const auto path_text = path.string();
    if (path_text.find('\n') != std::string::npos ||
        path_text.find('\r') != std::string::npos) {
      local_fail("path contains a newline");
    }
    std::error_code error;
    const auto absolute = std::filesystem::absolute(path, error);
    if (error || !paths.insert(absolute.lexically_normal()).second) {
      local_fail("train, validation, and output paths must be distinct");
    }
  }
  return options;
}

void require_regular_nonsymlink(const std::filesystem::path &path) {
  std::error_code error;
  const auto status = std::filesystem::symlink_status(path, error);
  if (error || std::filesystem::is_symlink(status) ||
      !std::filesystem::is_regular_file(status)) {
    local_fail("input must be a regular non-symlinked file: " + path.string());
  }
}

void write_exclusive(const std::filesystem::path &path,
                     const std::string &contents) {
  const int descriptor = ::open(
      path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (descriptor < 0) {
    local_fail("output must be absent and exclusively creatable: " +
               path.string() + ": " + std::strerror(errno));
  }
  int open_descriptor = descriptor;
  try {
    std::size_t offset = 0;
    while (offset < contents.size()) {
      const auto written = ::write(open_descriptor, contents.data() + offset,
                                   contents.size() - offset);
      if (written < 0 && errno == EINTR) {
        continue;
      }
      if (written <= 0) {
        local_fail("failed while writing exclusive report: " +
                   std::string(std::strerror(errno)));
      }
      offset += static_cast<std::size_t>(written);
    }
    if (::fsync(open_descriptor) != 0) {
      local_fail("failed while syncing exclusive report: " +
                 std::string(std::strerror(errno)));
    }
    if (::close(open_descriptor) != 0) {
      open_descriptor = -1;
      local_fail("failed while closing exclusive report: " +
                 std::string(std::strerror(errno)));
    }
    open_descriptor = -1;
  } catch (...) {
    if (open_descriptor >= 0) {
      (void)::close(open_descriptor);
    }
    (void)::unlink(path.c_str());
    throw;
  }
}

ConditionedAffineModel fit_conditioned_affine(const Dataset &dataset) {
  torch::NoGradGuard no_grad;
  if (dataset.anchor_begin != kTrainBegin || dataset.anchor_end != kTrainEnd ||
      dataset.features.dim() != 4 ||
      dataset.features.size(0) != kTrainEnd - kTrainBegin ||
      dataset.features.size(1) != kEdgeCount ||
      dataset.features.size(2) != kChannelCount ||
      dataset.features.size(3) != kFeatureWidth ||
      dataset.target.sizes() !=
          torch::IntArrayRef(
              {kTrainEnd - kTrainBegin, kEdgeCount, kChannelCount})) {
    local_fail("conditioned affine fit tensor contract mismatch");
  }

  const auto features = dataset.features;
  const auto target = dataset.target;
  const auto flat = features.reshape({-1, kFeatureWidth});
  const auto mean = flat.mean(0);
  const auto variance = (flat - mean).pow(2).mean(0);
  const auto standard_deviation = variance.sqrt();
  const auto inv_std = torch::where(standard_deviation > 1.0e-12,
                                    standard_deviation.reciprocal(),
                                    torch::ones_like(standard_deviation));
  const auto standardized = (features - mean.view({1, 1, 1, kFeatureWidth})) *
                            inv_std.view({1, 1, 1, kFeatureWidth});

  auto weights =
      torch::zeros({kEdgeCount, kChannelCount, kFeatureWidth}, torch::kFloat64);
  auto bias = torch::zeros({kEdgeCount, kChannelCount}, torch::kFloat64);
  double maximum_residual = 0.0;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    for (int64_t channel = 0; channel < kChannelCount; ++channel) {
      const auto x =
          standardized.select(1, edge).select(1, channel).contiguous();
      const auto y = target.select(1, edge).select(1, channel).contiguous();
      const auto x_mean = x.mean(0);
      const auto y_mean = y.mean();
      const auto centered_x = x - x_mean;
      const auto centered_y = y - y_mean;
      auto gram = centered_x.transpose(0, 1).matmul(centered_x);
      gram.diagonal(0, 0, 1).add_(static_cast<double>(x.size(0)) * kFixedRidge);
      const auto rhs =
          centered_x.transpose(0, 1).matmul(centered_y.unsqueeze(1));
      auto [cholesky, info] = at::linalg_cholesky_ex(gram, false, false);
      if (info.item<int64_t>() != 0) {
        local_fail("conditioned affine cholesky factorization failed");
      }
      const auto row = at::cholesky_solve(rhs, cholesky, false).squeeze(1);
      const auto residual = gram.matmul(row.unsqueeze(1)) - rhs;
      const double normalized_residual =
          residual.norm().item<double>() /
          std::max(rhs.norm().item<double>(), 1.0e-30);
      if (!std::isfinite(normalized_residual) || normalized_residual > 1.0e-7 ||
          !torch::isfinite(row).all().item<bool>()) {
        local_fail("conditioned affine residual or finiteness check failed");
      }
      maximum_residual = std::max(maximum_residual, normalized_residual);
      weights.select(0, edge).select(0, channel).copy_(row);
      bias.select(0, edge).select(0, channel).copy_(y_mean - x_mean.dot(row));
    }
  }
  return {.mean = mean.contiguous(),
          .inv_std = inv_std.contiguous(),
          .weights = weights.contiguous(),
          .bias = bias.contiguous(),
          .maximum_normalized_residual = maximum_residual,
          .coefficient_l2_norm = weights.norm().item<double>()};
}

torch::Tensor predict_float64(const ConditionedAffineModel &model,
                              const torch::Tensor &features) {
  torch::NoGradGuard no_grad;
  const auto standardized =
      (features - model.mean.view({1, 1, 1, kFeatureWidth})) *
      model.inv_std.view({1, 1, 1, kFeatureWidth});
  return (standardized *
          model.weights.view({1, kEdgeCount, kChannelCount, kFeatureWidth}))
             .sum(-1) +
         model.bias.view({1, kEdgeCount, kChannelCount});
}

StandardizedFeatures standardize_features(const Dataset &train,
                                          const Dataset &validation) {
  torch::NoGradGuard no_grad;
  const auto flat_train =
      train.features.to(torch::kFloat32).reshape({-1, kFeatureWidth});
  const auto flat_validation =
      validation.features.to(torch::kFloat32).reshape({-1, kFeatureWidth});
  const auto mean = flat_train.mean(0);
  const auto variance = (flat_train - mean).pow(2).mean(0);
  const auto standard_deviation = variance.sqrt();
  const auto clamped = standard_deviation < kFeatureStandardDeviationFloor;
  const auto scale =
      standard_deviation.clamp_min(kFeatureStandardDeviationFloor);
  StandardizedFeatures result{
      .train = ((flat_train - mean) / scale).contiguous(),
      .validation = ((flat_validation - mean) / scale).contiguous(),
      .mean = mean.contiguous(),
      .scale = scale.contiguous(),
      .clamped_coordinate_count = clamped.sum().item<int64_t>()};
  if (!torch::isfinite(result.train).all().item<bool>() ||
      !torch::isfinite(result.validation).all().item<bool>()) {
    local_fail("standardized feature arm contains nonfinite values");
  }
  return result;
}

StandardizedTarget standardize_target(const Dataset &train,
                                      const Dataset &validation) {
  torch::NoGradGuard no_grad;
  const auto mean = train.target.mean(0);
  const auto variance =
      (train.target - mean.view({1, kEdgeCount, kChannelCount})).pow(2).mean(0);
  const auto standard_deviation = variance.sqrt();
  const auto clamped = standard_deviation < kFeatureStandardDeviationFloor;
  const auto scale =
      standard_deviation.clamp_min(kFeatureStandardDeviationFloor);
  StandardizedTarget result{
      .train = ((train.target - mean.view({1, kEdgeCount, kChannelCount})) /
                scale.view({1, kEdgeCount, kChannelCount}))
                   .reshape({-1})
                   .to(torch::kFloat32)
                   .contiguous(),
      .validation =
          ((validation.target - mean.view({1, kEdgeCount, kChannelCount})) /
           scale.view({1, kEdgeCount, kChannelCount}))
              .reshape({-1})
              .to(torch::kFloat32)
              .contiguous(),
      .mean = mean.contiguous(),
      .scale = scale.contiguous(),
      .clamped_coordinate_count = clamped.sum().item<int64_t>()};
  if (!torch::isfinite(result.train).all().item<bool>() ||
      !torch::isfinite(result.validation).all().item<bool>()) {
    local_fail("standardized target contains nonfinite values");
  }
  return result;
}

torch::Tensor head_indices(int64_t anchor_count) {
  std::vector<int64_t> values;
  values.reserve(static_cast<std::size_t>(anchor_count * kHeadCount));
  for (int64_t anchor = 0; anchor < anchor_count; ++anchor) {
    (void)anchor;
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      for (int64_t channel = 0; channel < kChannelCount; ++channel) {
        values.push_back(channel * kEdgeCount + edge);
      }
    }
  }
  return torch::from_blob(values.data(), {static_cast<int64_t>(values.size())},
                          torch::kInt64)
      .clone();
}

Float32HeadMap
convert_to_standardized_float32_heads(const ConditionedAffineModel &model,
                                      const StandardizedFeatures &features,
                                      const StandardizedTarget &target) {
  torch::NoGradGuard no_grad;
  auto weights64 = torch::zeros({kHeadCount, kFeatureWidth}, torch::kFloat64);
  auto bias64 = torch::zeros({kHeadCount}, torch::kFloat64);
  const auto feature_mean64 = features.mean.to(torch::kFloat64);
  const auto feature_scale64 = features.scale.to(torch::kFloat64);
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    for (int64_t channel = 0; channel < kChannelCount; ++channel) {
      const int64_t head = channel * kEdgeCount + edge;
      const auto original_slope =
          model.weights.select(0, edge).select(0, channel) * model.inv_std;
      const auto target_scale = target.scale.select(0, edge).select(0, channel);
      weights64.select(0, head).copy_(original_slope * feature_scale64 /
                                      target_scale);
      const auto translated_bias =
          (model.bias.select(0, edge).select(0, channel) +
           original_slope.dot(feature_mean64 - model.mean) -
           target.mean.select(0, edge).select(0, channel)) /
          target_scale;
      bias64.select(0, head).copy_(translated_bias);
    }
  }
  Float32HeadMap result{.weights = weights64.to(torch::kFloat32).contiguous(),
                        .bias = bias64.to(torch::kFloat32).contiguous()};
  if (!torch::isfinite(result.weights).all().item<bool>() ||
      !torch::isfinite(result.bias).all().item<bool>()) {
    local_fail("converted float32 affine map contains nonfinite values");
  }
  return result;
}

torch::Tensor gather_direct_heads(const torch::Tensor &features,
                                  const torch::Tensor &heads,
                                  const Float32HeadMap &mapping) {
  torch::NoGradGuard no_grad;
  const auto all_heads =
      features.matmul(mapping.weights.transpose(0, 1)) + mapping.bias;
  return all_heads.gather(1, heads.unsqueeze(1)).squeeze(1).contiguous();
}

class PairedGeluMlpImpl : public torch::nn::Module {
public:
  PairedGeluMlpImpl() {
    input_ = register_module("input",
                             torch::nn::Linear(kFeatureWidth, kHiddenWidth));
    hidden_ = register_module("hidden",
                              torch::nn::Linear(kHiddenWidth, kHiddenWidth));
    output_ =
        register_module("output", torch::nn::Linear(kHiddenWidth, kHeadCount));
  }

  void inject(const Float32HeadMap &mapping) {
    torch::NoGradGuard no_grad;
    input_->weight.zero_();
    input_->bias.zero_();
    hidden_->weight.zero_();
    hidden_->bias.zero_();
    output_->weight.zero_();
    output_->bias.zero_();
    for (int64_t head = 0; head < kHeadCount; ++head) {
      const int64_t positive = 2 * head;
      const int64_t negative = positive + 1;
      input_->weight.select(0, positive).copy_(mapping.weights.select(0, head));
      input_->weight.select(0, negative)
          .copy_(-mapping.weights.select(0, head));
      input_->bias.select(0, positive).copy_(mapping.bias.select(0, head));
      input_->bias.select(0, negative).copy_(-mapping.bias.select(0, head));
      hidden_->weight.select(0, positive).select(0, positive).fill_(1.0);
      hidden_->weight.select(0, positive).select(0, negative).fill_(-1.0);
      hidden_->weight.select(0, negative).select(0, positive).fill_(-1.0);
      hidden_->weight.select(0, negative).select(0, negative).fill_(1.0);
      output_->weight.select(0, head).select(0, positive).fill_(1.0);
      output_->weight.select(0, head).select(0, negative).fill_(-1.0);
    }
  }

  torch::Tensor forward(const torch::Tensor &features,
                        const torch::Tensor &heads) {
    const auto all_heads = output_->forward(
        torch::gelu(hidden_->forward(torch::gelu(input_->forward(features)))));
    return all_heads.gather(1, heads.unsqueeze(1)).squeeze(1);
  }

private:
  torch::nn::Linear input_{nullptr};
  torch::nn::Linear hidden_{nullptr};
  torch::nn::Linear output_{nullptr};
};
TORCH_MODULE(PairedGeluMlp);

class GatheredLinearImpl : public torch::nn::Module {
public:
  GatheredLinearImpl() {
    linear_ =
        register_module("linear", torch::nn::Linear(kFeatureWidth, kHeadCount));
  }

  torch::Tensor forward(const torch::Tensor &features,
                        const torch::Tensor &heads) {
    return linear_->forward(features).gather(1, heads.unsqueeze(1)).squeeze(1);
  }

private:
  torch::nn::Linear linear_{nullptr};
};
TORCH_MODULE(GatheredLinear);

BatchSchedule make_batch_schedule(int64_t row_count) {
  if (row_count <= 0) {
    local_fail("cannot create a schedule for an empty training tensor");
  }
  std::mt19937_64 generator(static_cast<std::uint64_t>(kSeed));
  std::uniform_int_distribution<int64_t> distribution(0, row_count - 1);
  std::vector<int64_t> values(static_cast<std::size_t>(kSteps * kBatchSize));
  std::uint64_t fingerprint = 1469598103934665603ULL;
  for (auto &value : values) {
    value = distribution(generator);
    const std::uint64_t encoded = static_cast<std::uint64_t>(value);
    for (int byte = 0; byte < 8; ++byte) {
      fingerprint ^= (encoded >> (8 * byte)) & 0xffULL;
      fingerprint *= 1099511628211ULL;
    }
  }
  std::ostringstream rendered;
  rendered << std::hex << std::setfill('0') << std::setw(16) << fingerprint;
  return {.indices = torch::from_blob(values.data(), {kSteps, kBatchSize},
                                      torch::kInt64)
                         .clone(),
          .fingerprint = rendered.str()};
}

void require_finite_linear(const GatheredLinear &model,
                           bool require_gradients) {
  for (const auto &parameter : model->parameters()) {
    if (!torch::isfinite(parameter).all().item<bool>()) {
      local_fail("direct linear model contains a nonfinite parameter");
    }
    if (require_gradients &&
        (!parameter.grad().defined() ||
         !torch::isfinite(parameter.grad()).all().item<bool>())) {
      local_fail("direct linear model contains an absent/nonfinite gradient");
    }
  }
}

double full_standardized_mse(GatheredLinear &model,
                             const torch::Tensor &features,
                             const torch::Tensor &target,
                             const torch::Tensor &heads) {
  torch::NoGradGuard no_grad;
  model->eval();
  const double value =
      torch::mse_loss(model->forward(features, heads), target).item<double>();
  if (!(value > 0.0) || !std::isfinite(value)) {
    local_fail("full-train standardized MSE must be positive and finite");
  }
  return value;
}

LinearTrainingSummary
train_direct_linear(GatheredLinear &model, const torch::Tensor &features,
                    const torch::Tensor &target, const torch::Tensor &heads,
                    const BatchSchedule &schedule, int64_t anchor_count) {
  LinearTrainingSummary summary;
  summary.initial_full_train_standardized_mse =
      full_standardized_mse(model, features, target, heads);
  require_finite_linear(model, false);

  torch::optim::AdamOptions adam_options(kLearningRate);
  adam_options.betas(std::make_tuple(kAdamBeta1, kAdamBeta2));
  adam_options.eps(kAdamEpsilon);
  adam_options.weight_decay(kWeightDecay);
  torch::optim::Adam optimizer(model->parameters(), adam_options);
  model->train();
  for (int64_t step = 0; step < kSteps; ++step) {
    const auto batch = schedule.indices.select(0, step);
    const auto prediction = model->forward(features.index_select(0, batch),
                                           heads.index_select(0, batch));
    const auto loss =
        torch::mse_loss(prediction, target.index_select(0, batch));
    if (!torch::isfinite(loss).item<bool>()) {
      local_fail("direct linear training produced a nonfinite loss");
    }
    optimizer.zero_grad();
    loss.backward();
    require_finite_linear(model, true);
    const double gradient_norm = torch::nn::utils::clip_grad_norm_(
        model->parameters(), kGradientClipNorm);
    if (!std::isfinite(gradient_norm)) {
      local_fail("direct linear training produced a nonfinite gradient norm");
    }
    summary.maximum_preclip_gradient_norm =
        std::max(summary.maximum_preclip_gradient_norm, gradient_norm);
    if (gradient_norm > kGradientClipNorm) {
      ++summary.clipped_step_count;
    }
    optimizer.step();
    require_finite_linear(model, false);
    summary.last_minibatch_loss = loss.item<double>();
    ++summary.optimizer_steps;
  }
  if (summary.optimizer_steps != kSteps) {
    local_fail("direct linear training did not complete its fixed schedule");
  }
  summary.final_full_train_standardized_mse =
      full_standardized_mse(model, features, target, heads);

  torch::NoGradGuard no_grad;
  const auto prediction =
      model->forward(features, heads)
          .reshape({anchor_count, kEdgeCount, kChannelCount});
  const auto target_cube =
      target.reshape({anchor_count, kEdgeCount, kChannelCount});
  for (int64_t head = 0; head < kHeadCount; ++head) {
    const int64_t edge = head % kEdgeCount;
    const int64_t channel = head / kEdgeCount;
    summary
        .per_head_full_train_standardized_mse[static_cast<std::size_t>(head)] =
        torch::mse_loss(prediction.select(1, edge).select(1, channel),
                        target_cube.select(1, edge).select(1, channel))
            .item<double>();
    const double head_mse =
        summary.per_head_full_train_standardized_mse[static_cast<std::size_t>(
            head)];
    if (!(head_mse > 0.0) || !std::isfinite(head_mse)) {
      local_fail("per-head full-train standardized MSE must be positive and "
                 "finite");
    }
  }
  return summary;
}

torch::Tensor original_units(const torch::Tensor &standardized_prediction,
                             int64_t anchor_count,
                             const StandardizedTarget &target) {
  torch::NoGradGuard no_grad;
  const auto cube =
      standardized_prediction.reshape({anchor_count, kEdgeCount, kChannelCount})
          .to(torch::kFloat64);
  const auto result = cube * target.scale.view({1, kEdgeCount, kChannelCount}) +
                      target.mean.view({1, kEdgeCount, kChannelCount});
  if (!torch::isfinite(result).all().item<bool>()) {
    local_fail("prediction conversion produced a nonfinite value");
  }
  return result.contiguous();
}

torch::Tensor standardized_units(const torch::Tensor &original_prediction,
                                 const StandardizedTarget &target) {
  return ((original_prediction -
           target.mean.view({1, kEdgeCount, kChannelCount})) /
          target.scale.view({1, kEdgeCount, kChannelCount}))
      .contiguous();
}

RouteMetrics evaluate_route(const torch::Tensor &prediction,
                            const torch::Tensor &target) {
  RouteMetrics result;
  result.aggregate = summarize(observe(prediction, target));
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    result.channels[static_cast<std::size_t>(channel)] = summarize(observe(
        prediction.narrow(2, channel, 1), target.narrow(2, channel, 1)));
  }
  return result;
}

void emit_route(std::ostream &output, const std::string &prefix,
                const RouteMetrics &metrics) {
  emit_metric(output, prefix, metrics.aggregate);
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    emit_metric(output, prefix + ".channel_" + std::to_string(channel),
                metrics.channels[static_cast<std::size_t>(channel)]);
  }
}

double max_abs_delta(const torch::Tensor &lhs, const torch::Tensor &rhs) {
  if (lhs.sizes() != rhs.sizes()) {
    local_fail("prediction delta shape mismatch");
  }
  return (lhs.to(torch::kFloat64) - rhs.to(torch::kFloat64))
      .abs()
      .max()
      .item<double>();
}

double standardized_mse(const torch::Tensor &prediction,
                        const torch::Tensor &target) {
  if (prediction.numel() != target.numel()) {
    local_fail("standardized MSE shape mismatch");
  }
  const double value =
      torch::mse_loss(prediction.reshape({-1}).to(torch::kFloat64),
                      target.reshape({-1}).to(torch::kFloat64))
          .item<double>();
  if (!(value > 0.0) || !std::isfinite(value)) {
    local_fail("standardized MSE must be positive and finite");
  }
  return value;
}

std::array<double, kHeadCount>
per_head_standardized_mse(const torch::Tensor &prediction,
                          const torch::Tensor &target, int64_t anchor_count) {
  const auto prediction_cube =
      prediction.reshape({anchor_count, kEdgeCount, kChannelCount});
  const auto target_cube =
      target.reshape({anchor_count, kEdgeCount, kChannelCount});
  std::array<double, kHeadCount> result{};
  for (int64_t head = 0; head < kHeadCount; ++head) {
    const int64_t edge = head % kEdgeCount;
    const int64_t channel = head / kEdgeCount;
    result[static_cast<std::size_t>(head)] =
        standardized_mse(prediction_cube.select(1, edge).select(1, channel),
                         target_cube.select(1, edge).select(1, channel));
  }
  return result;
}

void run(const LocalOptions &options) {
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
  at::globalContext().setDeterministicFillUninitializedMemory(true);

  require_regular_nonsymlink(options.train_input);
  require_regular_nonsymlink(options.validation_input);
  const auto train = read_probe(options.train_input, kTrainBegin, kTrainEnd,
                                kRepresentationSpec);
  const auto validation = read_probe(options.validation_input, kValidationBegin,
                                     kValidationEnd, kRepresentationSpec);

  const auto affine = fit_conditioned_affine(train);
  const auto float64_train = predict_float64(affine, train.features);
  const auto float64_validation = predict_float64(affine, validation.features);

  const auto features = standardize_features(train, validation);
  const auto target = standardize_target(train, validation);
  const auto train_heads = head_indices(kTrainEnd - kTrainBegin);
  const auto validation_heads = head_indices(kValidationEnd - kValidationBegin);
  const auto head_map =
      convert_to_standardized_float32_heads(affine, features, target);

  const auto float32_train_standardized =
      gather_direct_heads(features.train, train_heads, head_map);
  const auto float32_validation_standardized =
      gather_direct_heads(features.validation, validation_heads, head_map);
  const auto float32_train = original_units(float32_train_standardized,
                                            kTrainEnd - kTrainBegin, target);
  const auto float32_validation =
      original_units(float32_validation_standardized,
                     kValidationEnd - kValidationBegin, target);

  torch::manual_seed(kSeed);
  PairedGeluMlp injected;
  injected->inject(head_map);
  injected->eval();
  torch::Tensor gelu_train_standardized;
  torch::Tensor gelu_validation_standardized;
  {
    torch::NoGradGuard no_grad;
    gelu_train_standardized =
        injected->forward(features.train, train_heads).contiguous();
    gelu_validation_standardized =
        injected->forward(features.validation, validation_heads).contiguous();
  }
  const auto gelu_train =
      original_units(gelu_train_standardized, kTrainEnd - kTrainBegin, target);
  const auto gelu_validation = original_units(
      gelu_validation_standardized, kValidationEnd - kValidationBegin, target);

  const auto float64_train_standardized =
      standardized_units(float64_train, target);
  const auto float64_validation_standardized =
      standardized_units(float64_validation, target);
  const double direct_delta_train = max_abs_delta(
      float64_train_standardized.reshape({-1}), float32_train_standardized);
  const double direct_delta_validation =
      max_abs_delta(float64_validation_standardized.reshape({-1}),
                    float32_validation_standardized);
  const double gelu_delta_train =
      max_abs_delta(float32_train_standardized, gelu_train_standardized);
  const double gelu_delta_validation = max_abs_delta(
      float32_validation_standardized, gelu_validation_standardized);

  const auto schedule = make_batch_schedule(features.train.size(0));
  torch::manual_seed(kSeed);
  GatheredLinear linear;
  const auto training =
      train_direct_linear(linear, features.train, target.train, train_heads,
                          schedule, kTrainEnd - kTrainBegin);
  torch::Tensor linear_train_standardized;
  torch::Tensor linear_validation_standardized;
  {
    torch::NoGradGuard no_grad;
    linear->eval();
    linear_train_standardized =
        linear->forward(features.train, train_heads).contiguous();
    linear_validation_standardized =
        linear->forward(features.validation, validation_heads).contiguous();
  }
  const auto linear_train = original_units(linear_train_standardized,
                                           kTrainEnd - kTrainBegin, target);
  const auto linear_validation =
      original_units(linear_validation_standardized,
                     kValidationEnd - kValidationBegin, target);

  const auto float64_train_metrics =
      evaluate_route(float64_train, train.target);
  const auto float64_validation_metrics =
      evaluate_route(float64_validation, validation.target);
  const auto float32_train_metrics =
      evaluate_route(float32_train, train.target);
  const auto float32_validation_metrics =
      evaluate_route(float32_validation, validation.target);
  const auto gelu_train_metrics = evaluate_route(gelu_train, train.target);
  const auto gelu_validation_metrics =
      evaluate_route(gelu_validation, validation.target);
  const auto linear_train_metrics = evaluate_route(linear_train, train.target);
  const auto linear_validation_metrics =
      evaluate_route(linear_validation, validation.target);

  const double oracle_train_standardized_mse =
      standardized_mse(float64_train_standardized, target.train);
  const double linear_train_standardized_mse =
      standardized_mse(linear_train_standardized, target.train);
  const auto oracle_head_train_standardized_mse = per_head_standardized_mse(
      float64_train_standardized, target.train, kTrainEnd - kTrainBegin);
  const auto linear_head_train_standardized_mse = per_head_standardized_mse(
      linear_train_standardized, target.train, kTrainEnd - kTrainBegin);
  const double aggregate_standardized_mse_ratio =
      linear_train_standardized_mse / oracle_train_standardized_mse;
  if (!(aggregate_standardized_mse_ratio > 0.0) ||
      !std::isfinite(aggregate_standardized_mse_ratio)) {
    local_fail("direct-linear/oracle aggregate MSE ratio must be positive and "
               "finite");
  }
  double maximum_head_standardized_mse_ratio = 0.0;
  std::array<double, kHeadCount> head_standardized_mse_ratios{};
  bool every_head_ratio_within_recovery_gate = true;
  for (int64_t head = 0; head < kHeadCount; ++head) {
    const auto index = static_cast<std::size_t>(head);
    const double ratio = linear_head_train_standardized_mse[index] /
                         oracle_head_train_standardized_mse[index];
    if (!(ratio > 0.0) || !std::isfinite(ratio)) {
      local_fail(
          "direct-linear/oracle head MSE ratio must be positive and finite");
    }
    head_standardized_mse_ratios[index] = ratio;
    maximum_head_standardized_mse_ratio =
        std::max(maximum_head_standardized_mse_ratio, ratio);
    every_head_ratio_within_recovery_gate =
        every_head_ratio_within_recovery_gate && ratio <= 1.10;
  }

  const bool direct_float32_parity_pass =
      std::max(direct_delta_train, direct_delta_validation) <=
      kDirectFloat32ParityTolerance;
  const bool paired_gelu_parity_pass =
      std::max(gelu_delta_train, gelu_delta_validation) <=
      kPairedGeluParityTolerance;
  if (!std::isfinite(training.initial_full_train_standardized_mse) ||
      !std::isfinite(training.final_full_train_standardized_mse) ||
      !std::isfinite(training.last_minibatch_loss) ||
      !std::isfinite(training.maximum_preclip_gradient_norm) ||
      training.initial_full_train_standardized_mse <= 0.0 ||
      training.final_full_train_standardized_mse <= 0.0 ||
      training.last_minibatch_loss < 0.0 ||
      training.maximum_preclip_gradient_norm < 0.0 ||
      training.clipped_step_count < 0 ||
      training.clipped_step_count > training.optimizer_steps) {
    local_fail("direct linear training diagnostics are invalid or nonfinite");
  }
  const bool linear_recovery =
      aggregate_standardized_mse_ratio <= 1.05 &&
      every_head_ratio_within_recovery_gate &&
      linear_train_metrics.aggregate.direction >=
          float64_train_metrics.aggregate.direction - 0.01 &&
      linear_train_metrics.aggregate.rank >=
          float64_train_metrics.aggregate.rank - 0.01 &&
      linear_train_metrics.aggregate.correlation >=
          float64_train_metrics.aggregate.correlation - 0.01 &&
      linear_train_metrics.aggregate.rmse_target_rms_ratio <=
          float64_train_metrics.aggregate.rmse_target_rms_ratio + 0.05;
  const bool linear_clear_failure =
      !linear_recovery && (aggregate_standardized_mse_ratio >= 1.25 ||
                           maximum_head_standardized_mse_ratio >= 1.50);
  std::string_view classification = "optimizer_localization_inconclusive";
  if (!direct_float32_parity_pass) {
    classification = "float32_conditioning_failure";
  } else if (!paired_gelu_parity_pass) {
    classification = "paired_gelu_execution_failure";
  } else if (linear_recovery) {
    classification = "deep_parameterization_or_optimization_failure";
  } else if (linear_clear_failure) {
    classification = "direct_linear_adam_optimizer_failure";
  }

  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::setprecision(17);
  output << "schema_id=" << kSchema << '\n';
  output << "status=complete\n";
  output << "benchmark_id=synthetic_continuous_graph_v2\n";
  output << "diagnostic_phase=affine_injection_optimizer_localization\n";
  output << "diagnostic_authority=development_only\n";
  output << "benchmark_acceptance_authority=false\n";
  output << "classification=" << classification << '\n';
  output << "train_input=" << options.train_input.string() << '\n';
  output << "validation_input=" << options.validation_input.string() << '\n';
  output << "probe_kind=representation\n";
  output << "probe_record_schema=" << kRepresentationSpec.record_schema << '\n';
  output << "train_probe_rows=" << train.rows << '\n';
  output << "validation_probe_rows=" << validation.rows << '\n';
  output << "certified_probe_rows=0\n";
  output << "fit_anchor_range=[0,2496)\n";
  output << "validation_anchor_range=[2560,2816)\n";
  output << "maximum_anchor_read=2815\n";
  output << "final_holdout_begin=3328\n";
  output << "validation_read_by_trainer=false\n";
  output << "validation_driven_choice=false\n";
  output << "representation_forward_executed=false\n";
  output << "checkpoint_written=false\n";
  output << "certified_input_access=false\n";
  output << "final_holdout_access=false\n";
  output << "policy_access=false\n";
  output << "fixed_ridge=" << kFixedRidge << '\n';
  output << "ridge_selection=false\n";
  output << "head_index_formula=channel*3+edge\n";
  output << "flat_row_order=anchor,edge,channel\n";
  output << "direct_architecture=Linear(96,9)+gather(channel*3+edge)\n";
  output << "injected_architecture=Linear(96,128)+GELU+Linear(128,128)+GELU+"
            "Linear(128,9)+gather(channel*3+edge)\n";
  output << "device=cpu\n";
  output << "feature_dtype=float32\n";
  output << "target_training_dtype=float32\n";
  output << "metric_dtype=float64\n";
  output << "oracle_solver_dtype=float64\n";
  output << "deterministic_algorithms=true\n";
  output << "deterministic_cudnn=true\n";
  output << "deterministic_fill_uninitialized_memory=true\n";
  output << "intraop_threads=1\n";
  output << "interop_threads=1\n";
  output << "float64_solver=float64_centered_cholesky_ridge\n";
  output
      << "float32_feature_standardization=train_core_all_edges_all_channels\n";
  output << "target_standardization=train_core_per_edge_channel\n";
  output << "gelu_identity=GELU(z)-GELU(-z)=z\n";
  output << "gelu_injected_hidden_units_per_layer=18\n";
  output << "gelu_unused_hidden_units_per_layer=110\n";
  output << "zero_optimizer_ladder_optimizer_steps=0\n";
  output << "feature_standardization_clamped_coordinate_count="
         << features.clamped_coordinate_count << '\n';
  output << "target_standardization_clamped_coordinate_count="
         << target.clamped_coordinate_count << '\n';
  output << "affine_maximum_normalized_residual="
         << affine.maximum_normalized_residual << '\n';
  output << "affine_coefficient_l2_norm=" << affine.coefficient_l2_norm << '\n';
  output << "oracle_phase2a_reference_validation=external_runner_required\n";
  output << "direct_float32_parity_tolerance_standardized_target_units="
         << kDirectFloat32ParityTolerance << '\n';
  output << "paired_gelu_parity_tolerance_standardized_target_units="
         << kPairedGeluParityTolerance << '\n';
  output << "direct_float32_parity_pass="
         << (direct_float32_parity_pass ? "true" : "false") << '\n';
  output << "paired_gelu_parity_pass="
         << (paired_gelu_parity_pass ? "true" : "false") << '\n';
  output << "delta.float64_oracle_vs_direct_float32.train.standardized_target_"
            "units_max_abs="
         << direct_delta_train << '\n';
  output << "delta.float64_oracle_vs_direct_float32.validation.standardized_"
            "target_units_max_abs="
         << direct_delta_validation << '\n';
  output << "delta.direct_float32_vs_paired_gelu.train.standardized_target_"
            "units_max_abs="
         << gelu_delta_train << '\n';
  output << "delta.direct_float32_vs_paired_gelu.validation.standardized_"
            "target_units_max_abs="
         << gelu_delta_validation << '\n';
  output << "delta.float64_oracle_vs_direct_float32.train.original_units_max_"
            "abs="
         << max_abs_delta(float64_train, float32_train) << '\n';
  output << "delta.float64_oracle_vs_direct_float32.validation.original_units_"
            "max_abs="
         << max_abs_delta(float64_validation, float32_validation) << '\n';
  output << "delta.direct_float32_vs_paired_gelu.train.original_units_max_abs="
         << max_abs_delta(float32_train, gelu_train) << '\n';
  output << "delta.direct_float32_vs_paired_gelu.validation.original_units_max_"
            "abs="
         << max_abs_delta(float32_validation, gelu_validation) << '\n';
  output << "delta.float64_oracle_vs_paired_gelu.train.original_units_max_abs="
         << max_abs_delta(float64_train, gelu_train) << '\n';
  output << "delta.float64_oracle_vs_paired_gelu.validation.original_units_max_"
            "abs="
         << max_abs_delta(float64_validation, gelu_validation) << '\n';

  emit_route(output, "route.float64_oracle.train", float64_train_metrics);
  emit_route(output, "route.float64_oracle.validation",
             float64_validation_metrics);
  emit_route(output, "route.direct_float32.train", float32_train_metrics);
  emit_route(output, "route.direct_float32.validation",
             float32_validation_metrics);
  emit_route(output, "route.paired_gelu_injected.train", gelu_train_metrics);
  emit_route(output, "route.paired_gelu_injected.validation",
             gelu_validation_metrics);
  emit_route(output, "route.direct_linear_adam_seed31.train",
             linear_train_metrics);
  emit_route(output, "route.direct_linear_adam_seed31.validation",
             linear_validation_metrics);

  output << "route.float64_oracle.train.standardized_mse="
         << oracle_train_standardized_mse << '\n';
  output << "route.direct_linear_adam_seed31.train.standardized_mse="
         << linear_train_standardized_mse << '\n';
  for (int64_t head = 0; head < kHeadCount; ++head) {
    output << "route.float64_oracle.train.head_" << head << ".standardized_mse="
           << oracle_head_train_standardized_mse[static_cast<std::size_t>(head)]
           << '\n';
    output << "route.direct_linear_adam_seed31.train.head_" << head
           << ".standardized_mse="
           << linear_head_train_standardized_mse[static_cast<std::size_t>(head)]
           << '\n';
    output << "direct_linear_adam_to_oracle_train.head_" << head
           << ".standardized_mse_ratio="
           << head_standardized_mse_ratios[static_cast<std::size_t>(head)]
           << '\n';
  }
  output << "direct_linear_adam_to_oracle_train_standardized_mse_ratio="
         << aggregate_standardized_mse_ratio << '\n';
  output << "direct_linear_adam_to_oracle_train_maximum_head_standardized_"
            "mse_ratio="
         << maximum_head_standardized_mse_ratio << '\n';

  output << "affine_oracle_grouped_fit_count=1\n";
  output << "affine_oracle_head_solve_count=9\n";
  output << "optimizer_fits_completed=1\n";
  output << "total_train_fit_procedures=2\n";
  output << "optimizer_steps=" << training.optimizer_steps << '\n';
  output << "seed=" << kSeed << '\n';
  output << "steps_per_fit=" << kSteps << '\n';
  output << "batch_size=" << kBatchSize << '\n';
  output << "learning_rate=" << kLearningRate << '\n';
  output << "adam_beta1=" << kAdamBeta1 << '\n';
  output << "adam_beta2=" << kAdamBeta2 << '\n';
  output << "adam_epsilon=" << kAdamEpsilon << '\n';
  output << "weight_decay=" << kWeightDecay << '\n';
  output << "gradient_clip_norm=" << kGradientClipNorm << '\n';
  output << "optimizer=Adam\n";
  output << "batch_sampling=mt19937_64_uniform_with_replacement\n";
  output << "early_stopping=false\n";
  output << "seed_selection=false\n";
  output << "hyperparameter_search=false\n";
  output << "retry=false\n";
  output << "refit=false\n";
  output << "batch_schedule_fingerprint=" << schedule.fingerprint << '\n';
  output << "direct_linear_adam.initial_full_train_standardized_mse="
         << training.initial_full_train_standardized_mse << '\n';
  output << "direct_linear_adam.final_full_train_standardized_mse="
         << training.final_full_train_standardized_mse << '\n';
  output << "direct_linear_adam.last_minibatch_loss="
         << training.last_minibatch_loss << '\n';
  output << "direct_linear_adam.maximum_preclip_gradient_norm="
         << training.maximum_preclip_gradient_norm << '\n';
  output << "direct_linear_adam.clipped_step_count="
         << training.clipped_step_count << '\n';
  for (int64_t head = 0; head < kHeadCount; ++head) {
    const int64_t edge = head % kEdgeCount;
    const int64_t channel = head / kEdgeCount;
    output << "direct_linear_adam.full_train_standardized_mse.head_" << head
           << ".edge_" << edge << ".channel_" << channel << '='
           << training.per_head_full_train_standardized_mse
                  [static_cast<std::size_t>(head)]
           << '\n';
  }
  output << "direct_linear_adam_recovery_gate_pass="
         << (linear_recovery ? "true" : "false") << '\n';
  output << "direct_linear_adam_clear_failure_gate_pass="
         << (linear_clear_failure ? "true" : "false") << '\n';
  output << "direct_linear_adam_recovery_gate=train_aggregate_mse_ratio<=1.05,"
            "each_train_head_mse_ratio<=1.10,train_direction>=oracle-0.01,"
            "train_rank>=oracle-0.01,train_correlation>=oracle-0.01,"
            "train_rmse_ratio<=oracle+0.05\n";
  output << "direct_linear_adam_clear_failure_gate=!recovery_and_(train_"
            "aggregate_mse_ratio>=1.25_or_max_train_head_mse_ratio>=1.50)\n";
  if (!output) {
    local_fail("failed while rendering report");
  }
  write_exclusive(options.output, output.str());
}

} // namespace

int main(int argc, char **argv) {
  try {
    std::locale::global(std::locale::classic());
    run(parse_local_options(argc, argv));
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "frozen representation affine injection optimizer "
                 "localization probe: "
              << error.what() << '\n';
    return 1;
  }
}
