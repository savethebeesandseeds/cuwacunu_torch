// Development-only Project Clear Signal Phase 2B corrected-control matched nonlinear probe.
//
// Reuse the frozen affine probe's row vocabulary, metric definitions, and
// strong/partial gates. Rename its entry point so this translation unit owns
// the single Phase 2B invocation and its exactly six in-memory fits.
#define main cuwacunu_embedded_frozen_representation_affine_probe_main
#include "frozen_representation_affine_probe.cpp"
#undef main

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <random>
#include <unistd.h>

namespace {

constexpr std::string_view kPhase2BSchema =
    "synthetic_v2_matched_nonlinear_sufficiency_corrected_control_development_v1";
constexpr std::string_view kRawRecordSchema =
    "kikijyeba.synthetic.raw_nodelift_edge_feature_probe.corrected_control.v1";
constexpr int64_t kFeatureWidth = 96;
constexpr int64_t kHiddenWidth = 128;
constexpr int64_t kHeadCount = kEdgeCount * kChannelCount;
constexpr int64_t kSteps = 3500;
constexpr int64_t kBatchSize = 64;
constexpr double kLearningRate = 1.0e-3;
constexpr double kAdamBeta1 = 0.9;
constexpr double kAdamBeta2 = 0.999;
constexpr double kAdamEpsilon = 1.0e-8;
constexpr double kWeightDecay = 0.0;
constexpr double kGradClipNorm = 5.0;
constexpr double kStandardDeviationFloor = 1.0e-8;
constexpr std::array<int64_t, 3> kSeeds{31, 47, 73};
// Structural right-alignment capacities only. Values inside a capacity may
// also be zero and are not asserted to be active observations.
constexpr std::array<int64_t, kChannelCount> kRawConfiguredCapacities{4, 10, 30};

struct Phase2BOptions {
  bool development_only{false};
  std::filesystem::path representation_train_input;
  std::filesystem::path representation_validation_input;
  std::filesystem::path raw_train_input;
  std::filesystem::path raw_validation_input;
  std::filesystem::path output;
};

struct MatchedDataset {
  torch::Tensor representation_features; // [A,E,C,96], CPU float32.
  torch::Tensor raw_features;            // [A,E,C,96], CPU float32.
  torch::Tensor target;                  // [A,E,C], CPU float64.
  int64_t anchor_begin{0};
  int64_t anchor_end{0};
  int64_t rows{0};
  double representation_identity_max_abs_delta{0.0};
  double raw_identity_max_abs_delta{0.0};
  int64_t raw_configured_capacity_padding_zero_values_checked{0};
};

struct StandardizedArm {
  torch::Tensor train; // [N,96], CPU float32.
  torch::Tensor validation;
  torch::Tensor mean;  // [96], CPU float32.
  torch::Tensor scale; // [96], CPU float32.
  int64_t clamped_coordinate_count{0};
};

struct StandardizedTarget {
  torch::Tensor train;      // [N], CPU float32.
  torch::Tensor validation; // [N], CPU float32.
  torch::Tensor mean;       // [E,C], CPU float64.
  torch::Tensor scale;      // [E,C], CPU float64.
  int64_t clamped_coordinate_count{0};
};

struct TrainingSummary {
  double last_loss{0.0};
  double maximum_gradient_norm{0.0};
  int64_t optimizer_steps{0};
};

struct SeedEvaluation {
  int64_t seed{0};
  TrainingSummary training{};
  MetricSummary train{};
  MetricSummary validation{};
  std::array<MetricSummary, kChannelCount> train_channels{};
  std::array<MetricSummary, kChannelCount> validation_channels{};
  bool validation_strong_gate_pass{false};
  bool validation_partial_gate_pass{false};
};

struct ArmEvaluation {
  std::string name;
  std::array<SeedEvaluation, kSeeds.size()> seeds{};
  MetricSummary median_train{};
  MetricSummary median_validation{};
  std::array<MetricSummary, kChannelCount> median_train_channels{};
  std::array<MetricSummary, kChannelCount> median_validation_channels{};
  int64_t strong_seed_count{0};
  bool pass{false};
};

[[noreturn]] void phase2b_fail(const std::string &message) {
  throw std::runtime_error(message);
}

std::string phase2b_required_value(int argc, char **argv, int &index,
                                   const char *flag) {
  if (index + 1 >= argc) {
    phase2b_fail(std::string("missing value for ") + flag);
  }
  return argv[++index];
}

void require_regular_input(const std::filesystem::path &path) {
  std::error_code error;
  const auto status = std::filesystem::symlink_status(path, error);
  if (error || std::filesystem::is_symlink(status) ||
      !std::filesystem::is_regular_file(status)) {
    phase2b_fail("input must be a regular non-symlinked file: " +
                 path.string());
  }
}

Phase2BOptions parse_phase2b_options(int argc, char **argv) {
  Phase2BOptions options;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--development-only") {
      if (options.development_only) {
        phase2b_fail("duplicate --development-only");
      }
      options.development_only = true;
    } else if (argument == "--representation-train-input") {
      options.representation_train_input =
          phase2b_required_value(argc, argv, index, argument.c_str());
    } else if (argument == "--representation-validation-input") {
      options.representation_validation_input =
          phase2b_required_value(argc, argv, index, argument.c_str());
    } else if (argument == "--raw-train-input") {
      options.raw_train_input =
          phase2b_required_value(argc, argv, index, argument.c_str());
    } else if (argument == "--raw-validation-input") {
      options.raw_validation_input =
          phase2b_required_value(argc, argv, index, argument.c_str());
    } else if (argument == "--output") {
      options.output =
          phase2b_required_value(argc, argv, index, argument.c_str());
    } else if (argument == "--certified-input" ||
               argument == "--evaluation-input" ||
               argument == "--holdout-input" ||
               argument == "--final-input" ||
               argument == "--policy-input" ||
               argument == "--checkpoint-output" ||
               argument == "--refit-input" ||
               argument == "--selection-lock") {
      phase2b_fail("forbidden Phase 2B argument: " + argument);
    } else {
      phase2b_fail("unknown argument: " + argument);
    }
  }
  if (!options.development_only ||
      options.representation_train_input.empty() ||
      options.representation_validation_input.empty() ||
      options.raw_train_input.empty() ||
      options.raw_validation_input.empty() || options.output.empty()) {
    phase2b_fail(
        "--development-only and all four inputs plus --output are required");
  }

  std::set<std::filesystem::path> distinct_paths;
  const std::array paths{options.representation_train_input,
                         options.representation_validation_input,
                         options.raw_train_input,
                         options.raw_validation_input, options.output};
  for (const auto &path : paths) {
    if (!path.is_absolute()) {
      phase2b_fail("all Phase 2B paths must be absolute: " + path.string());
    }
    const auto normalized = path.lexically_normal();
    if (!distinct_paths.insert(normalized).second) {
      phase2b_fail("all Phase 2B inputs and output must be distinct");
    }
    const auto text = path.string();
    if (text.find('\n') != std::string::npos ||
        text.find('\r') != std::string::npos) {
      phase2b_fail("Phase 2B path contains a newline");
    }
  }
  require_regular_input(options.representation_train_input);
  require_regular_input(options.representation_validation_input);
  require_regular_input(options.raw_train_input);
  require_regular_input(options.raw_validation_input);

  std::error_code output_error;
  const auto output_status =
      std::filesystem::symlink_status(options.output, output_error);
  if (!output_error && output_status.type() !=
                           std::filesystem::file_type::not_found) {
    phase2b_fail("Phase 2B output must not already exist");
  }
  const auto output_parent = options.output.parent_path();
  const auto parent_status =
      std::filesystem::symlink_status(output_parent, output_error);
  if (output_error || std::filesystem::is_symlink(parent_status) ||
      !std::filesystem::is_directory(parent_status)) {
    phase2b_fail("Phase 2B output parent must be a non-symlinked directory");
  }
  return options;
}

std::vector<float> parse_feature_vector(const std::string &text,
                                        const char *arm) {
  const auto cells = split_exact(text, ';');
  if (static_cast<int64_t>(cells.size()) != kFeatureWidth) {
    phase2b_fail(std::string(arm) + " feature vector is not width 96");
  }
  std::vector<float> values(static_cast<std::size_t>(kFeatureWidth));
  for (int64_t feature = 0; feature < kFeatureWidth; ++feature) {
    const double parsed =
        parse_f64(cells[static_cast<std::size_t>(feature)], "feature_value");
    const float narrowed = static_cast<float>(parsed);
    if (!std::isfinite(narrowed)) {
      phase2b_fail(std::string(arm) + " feature narrows to nonfinite float32");
    }
    values[static_cast<std::size_t>(feature)] = narrowed;
  }
  return values;
}

double feature_identity_delta(const std::vector<float> &values) {
  double maximum = 0.0;
  for (int64_t feature = 0; feature < 32; ++feature) {
    const float expected = values[static_cast<std::size_t>(feature)] -
                           values[static_cast<std::size_t>(32 + feature)];
    maximum = std::max(
        maximum,
        std::fabs(static_cast<double>(
            expected - values[static_cast<std::size_t>(64 + feature)])));
  }
  return maximum;
}

MatchedDataset read_matched_probes(const std::filesystem::path &representation,
                                   const std::filesystem::path &raw,
                                   int64_t expected_begin,
                                   int64_t expected_end) {
  if (!((expected_begin == kTrainBegin && expected_end == kTrainEnd) ||
        (expected_begin == kValidationBegin &&
         expected_end == kValidationEnd))) {
    phase2b_fail("Phase 2B accepts only the exact train or validation range");
  }
  std::ifstream representation_input(representation);
  std::ifstream raw_input(raw);
  if (!representation_input || !raw_input) {
    phase2b_fail("failed to open a Phase 2B probe input");
  }
  representation_input.imbue(std::locale::classic());
  raw_input.imbue(std::locale::classic());

  std::string representation_line;
  std::string raw_line;
  if (!std::getline(representation_input, representation_line) ||
      representation_line != kProbeHeader ||
      !std::getline(raw_input, raw_line) || raw_line != kProbeHeader) {
    phase2b_fail("Phase 2B probe header mismatch");
  }

  const int64_t anchor_count = expected_end - expected_begin;
  const int64_t total_rows = anchor_count * kEdgeCount * kChannelCount;
  std::vector<float> representation_values(
      static_cast<std::size_t>(total_rows * kFeatureWidth),
      std::numeric_limits<float>::quiet_NaN());
  std::vector<float> raw_values(
      static_cast<std::size_t>(total_rows * kFeatureWidth),
      std::numeric_limits<float>::quiet_NaN());
  std::vector<double> targets(static_cast<std::size_t>(total_rows),
                              std::numeric_limits<double>::quiet_NaN());
  std::vector<bool> seen(static_cast<std::size_t>(total_rows), false);
  double representation_identity_delta = 0.0;
  double raw_identity_delta = 0.0;
  int64_t raw_configured_capacity_padding_zero_values_checked = 0;
  int64_t rows = 0;

  const auto cube_row = [](int64_t local_anchor, int64_t edge,
                           int64_t channel) {
    return (local_anchor * kEdgeCount + edge) * kChannelCount + channel;
  };

  for (;;) {
    const bool has_representation =
        static_cast<bool>(std::getline(representation_input,
                                       representation_line));
    const bool has_raw = static_cast<bool>(std::getline(raw_input, raw_line));
    if (has_representation != has_raw) {
      phase2b_fail("matched Phase 2B probes have different row counts");
    }
    if (!has_representation) {
      break;
    }
    if (representation_line.empty() || raw_line.empty()) {
      phase2b_fail("matched Phase 2B probe contains an empty row");
    }
    const auto representation_columns = split_exact(representation_line, ',');
    const auto raw_columns = split_exact(raw_line, ',');
    if (representation_columns.size() != 12 || raw_columns.size() != 12 ||
        representation_columns[0] != kRepresentationSpec.record_schema ||
        raw_columns[0] != kRawRecordSchema) {
      phase2b_fail("matched Phase 2B row schema mismatch");
    }
    for (std::size_t column = 1; column <= 10; ++column) {
      if (representation_columns[column] != raw_columns[column]) {
        phase2b_fail("Phase 2B coordinate/target columns differ at row " +
                     std::to_string(rows) + ", column " +
                     std::to_string(column));
      }
    }

    (void)parse_i64(representation_columns[1], "anchor_key");
    const int64_t anchor =
        parse_i64(representation_columns[2], "anchor_index");
    const int64_t anchor_local =
        parse_i64(representation_columns[3], "anchor_local_index");
    const int64_t edge =
        parse_i64(representation_columns[4], "edge_index");
    const int64_t channel =
        parse_i64(representation_columns[8], "channel_index");
    const double target = parse_f64(representation_columns[9],
                                    "target_edge_close_return");
    const int64_t feature_count =
        parse_i64(representation_columns[10], "feature_count");
    if (anchor < expected_begin || anchor >= expected_end ||
        anchor >= kFinalBegin || anchor_local < 0 || edge < 0 ||
        edge >= kEdgeCount || channel < 0 || channel >= kChannelCount ||
        feature_count != kFeatureWidth ||
        representation_columns[5] != kEdgeIds[static_cast<std::size_t>(edge)] ||
        representation_columns[6] != kBaseIds[static_cast<std::size_t>(edge)] ||
        representation_columns[7] != kQuoteId) {
      phase2b_fail("Phase 2B probe row domain mismatch");
    }
    const int64_t row = cube_row(anchor - expected_begin, edge, channel);
    if (seen[static_cast<std::size_t>(row)]) {
      phase2b_fail("duplicate Phase 2B probe coordinate");
    }
    seen[static_cast<std::size_t>(row)] = true;
    targets[static_cast<std::size_t>(row)] = target;

    const auto parsed_representation =
        parse_feature_vector(representation_columns[11], "representation");
    const auto parsed_raw = parse_feature_vector(raw_columns[11], "raw");
    representation_identity_delta =
        std::max(representation_identity_delta,
                 feature_identity_delta(parsed_representation));
    raw_identity_delta =
        std::max(raw_identity_delta, feature_identity_delta(parsed_raw));

    const int64_t leading =
        32 - kRawConfiguredCapacities[static_cast<std::size_t>(channel)];
    for (int64_t segment = 0; segment < 3; ++segment) {
      for (int64_t position = 0; position < leading; ++position) {
        const float value = parsed_raw[static_cast<std::size_t>(
            segment * 32 + position)];
        if (value != 0.0F) {
          phase2b_fail(
              "raw NodeLift history violates configured-capacity structural "
              "padding");
        }
        ++raw_configured_capacity_padding_zero_values_checked;
      }
    }

    for (int64_t feature = 0; feature < kFeatureWidth; ++feature) {
      const auto offset = static_cast<std::size_t>(row * kFeatureWidth +
                                                   feature);
      representation_values[offset] =
          parsed_representation[static_cast<std::size_t>(feature)];
      raw_values[offset] = parsed_raw[static_cast<std::size_t>(feature)];
    }
    ++rows;
  }
  if ((!representation_input.good() && !representation_input.eof()) ||
      (!raw_input.good() && !raw_input.eof())) {
    phase2b_fail("failed while reading matched Phase 2B probes");
  }
  if (rows != total_rows ||
      std::find(seen.begin(), seen.end(), false) != seen.end()) {
    phase2b_fail("matched Phase 2B probes are not a complete cube");
  }
  if (representation_identity_delta > 2.0e-6 ||
      raw_identity_delta > 2.0e-6) {
    phase2b_fail("Phase 2B base-minus-quote feature identity failed");
  }

  MatchedDataset dataset;
  dataset.representation_features =
      torch::from_blob(representation_values.data(),
                       {anchor_count, kEdgeCount, kChannelCount, kFeatureWidth},
                       torch::kFloat32)
          .clone();
  dataset.raw_features =
      torch::from_blob(raw_values.data(),
                       {anchor_count, kEdgeCount, kChannelCount, kFeatureWidth},
                       torch::kFloat32)
          .clone();
  dataset.target =
      torch::from_blob(targets.data(),
                       {anchor_count, kEdgeCount, kChannelCount},
                       torch::kFloat64)
          .clone();
  if (!torch::isfinite(dataset.representation_features).all().item<bool>() ||
      !torch::isfinite(dataset.raw_features).all().item<bool>() ||
      !torch::isfinite(dataset.target).all().item<bool>()) {
    phase2b_fail("Phase 2B input contains a nonfinite value");
  }
  dataset.anchor_begin = expected_begin;
  dataset.anchor_end = expected_end;
  dataset.rows = rows;
  dataset.representation_identity_max_abs_delta =
      representation_identity_delta;
  dataset.raw_identity_max_abs_delta = raw_identity_delta;
  dataset.raw_configured_capacity_padding_zero_values_checked =
      raw_configured_capacity_padding_zero_values_checked;
  return dataset;
}

StandardizedArm standardize_arm(const torch::Tensor &train,
                                const torch::Tensor &validation) {
  torch::NoGradGuard no_grad;
  if (train.dim() != 4 || validation.dim() != 4 ||
      train.size(-1) != kFeatureWidth ||
      validation.size(-1) != kFeatureWidth) {
    phase2b_fail("Phase 2B arm tensor shape mismatch");
  }
  const auto flat_train = train.reshape({-1, kFeatureWidth});
  const auto flat_validation = validation.reshape({-1, kFeatureWidth});
  const auto mean = flat_train.mean(0);
  const auto variance = (flat_train - mean).pow(2).mean(0);
  const auto standard_deviation = variance.sqrt();
  const auto clamped = standard_deviation < kStandardDeviationFloor;
  const auto scale = standard_deviation.clamp_min(kStandardDeviationFloor);
  StandardizedArm out;
  out.train = ((flat_train - mean) / scale).contiguous();
  out.validation = ((flat_validation - mean) / scale).contiguous();
  out.mean = mean.contiguous();
  out.scale = scale.contiguous();
  out.clamped_coordinate_count = clamped.sum().item<int64_t>();
  if (!torch::isfinite(out.train).all().item<bool>() ||
      !torch::isfinite(out.validation).all().item<bool>()) {
    phase2b_fail("Phase 2B standardized arm contains a nonfinite value");
  }
  return out;
}

StandardizedTarget standardize_target(const torch::Tensor &train,
                                      const torch::Tensor &validation) {
  torch::NoGradGuard no_grad;
  if (train.dim() != 3 || validation.dim() != 3 ||
      train.size(1) != kEdgeCount || train.size(2) != kChannelCount ||
      validation.size(1) != kEdgeCount ||
      validation.size(2) != kChannelCount) {
    phase2b_fail("Phase 2B target tensor shape mismatch");
  }
  const auto mean = train.mean(0);
  const auto variance = (train - mean.view({1, kEdgeCount, kChannelCount}))
                            .pow(2)
                            .mean(0);
  const auto standard_deviation = variance.sqrt();
  const auto clamped = standard_deviation < kStandardDeviationFloor;
  const auto scale = standard_deviation.clamp_min(kStandardDeviationFloor);
  StandardizedTarget out;
  out.train =
      ((train - mean.view({1, kEdgeCount, kChannelCount})) /
       scale.view({1, kEdgeCount, kChannelCount}))
          .reshape({-1})
          .to(torch::kFloat32)
          .contiguous();
  out.validation =
      ((validation - mean.view({1, kEdgeCount, kChannelCount})) /
       scale.view({1, kEdgeCount, kChannelCount}))
          .reshape({-1})
          .to(torch::kFloat32)
          .contiguous();
  out.mean = mean.contiguous();
  out.scale = scale.contiguous();
  out.clamped_coordinate_count = clamped.sum().item<int64_t>();
  if (!torch::isfinite(out.train).all().item<bool>() ||
      !torch::isfinite(out.validation).all().item<bool>()) {
    phase2b_fail("Phase 2B standardized target contains a nonfinite value");
  }
  return out;
}

torch::Tensor head_indices(int64_t anchor_count) {
  std::vector<int64_t> indices;
  indices.reserve(static_cast<std::size_t>(anchor_count * kEdgeCount *
                                           kChannelCount));
  for (int64_t anchor = 0; anchor < anchor_count; ++anchor) {
    (void)anchor;
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      for (int64_t channel = 0; channel < kChannelCount; ++channel) {
        indices.push_back(channel * kEdgeCount + edge);
      }
    }
  }
  return torch::from_blob(indices.data(),
                          {static_cast<int64_t>(indices.size())},
                          torch::kInt64)
      .clone();
}

class MatchedMlpImpl : public torch::nn::Module {
public:
  MatchedMlpImpl() {
    input_ = register_module("input", torch::nn::Linear(kFeatureWidth,
                                                         kHiddenWidth));
    hidden_ = register_module("hidden", torch::nn::Linear(kHiddenWidth,
                                                           kHiddenWidth));
    output_ = register_module("output",
                              torch::nn::Linear(kHiddenWidth, kHeadCount));
  }

  torch::Tensor forward(const torch::Tensor &features,
                        const torch::Tensor &heads) {
    if (features.dim() != 2 || features.size(1) != kFeatureWidth ||
        heads.dim() != 1 || heads.size(0) != features.size(0)) {
      phase2b_fail("Phase 2B MLP input shape mismatch");
    }
    auto all_heads = output_->forward(
        torch::gelu(hidden_->forward(torch::gelu(input_->forward(features)))));
    return all_heads.gather(1, heads.unsqueeze(1)).squeeze(1);
  }

private:
  torch::nn::Linear input_{nullptr};
  torch::nn::Linear hidden_{nullptr};
  torch::nn::Linear output_{nullptr};
};

TORCH_MODULE(MatchedMlp);

void require_identical_initialization(const MatchedMlp &lhs,
                                      const MatchedMlp &rhs) {
  const auto lhs_parameters = lhs->named_parameters(true);
  const auto rhs_parameters = rhs->named_parameters(true);
  if (lhs_parameters.size() != rhs_parameters.size()) {
    phase2b_fail("Phase 2B paired models have different parameter counts");
  }
  for (const auto &parameter : lhs_parameters) {
    if (!rhs_parameters.contains(parameter.key()) ||
        !torch::equal(parameter.value(), rhs_parameters[parameter.key()])) {
      phase2b_fail("Phase 2B paired model initialization differs");
    }
  }
}

struct BatchSchedule {
  torch::Tensor indices; // [steps,batch], int64.
  std::string fingerprint;
};

BatchSchedule make_batch_schedule(int64_t seed, int64_t row_count) {
  if (row_count <= 0) {
    phase2b_fail("cannot create a Phase 2B schedule for an empty dataset");
  }
  std::mt19937_64 generator(static_cast<std::uint64_t>(seed));
  std::uniform_int_distribution<int64_t> distribution(0, row_count - 1);
  std::vector<int64_t> values(
      static_cast<std::size_t>(kSteps * kBatchSize));
  std::uint64_t fingerprint = 1469598103934665603ULL;
  for (auto &value : values) {
    value = distribution(generator);
    std::uint64_t encoded = static_cast<std::uint64_t>(value);
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

void require_finite_parameters_and_gradients(const MatchedMlp &model,
                                             bool require_gradients) {
  for (const auto &parameter : model->parameters()) {
    if (!torch::isfinite(parameter).all().item<bool>()) {
      phase2b_fail("Phase 2B model contains a nonfinite parameter");
    }
    if (require_gradients) {
      if (!parameter.grad().defined() ||
          !torch::isfinite(parameter.grad()).all().item<bool>()) {
        phase2b_fail("Phase 2B model contains an absent/nonfinite gradient");
      }
    }
  }
}

TrainingSummary train_once(MatchedMlp &model,
                           const torch::Tensor &features,
                           const torch::Tensor &target,
                           const torch::Tensor &heads,
                           const BatchSchedule &schedule) {
  if (features.dim() != 2 || features.size(1) != kFeatureWidth ||
      target.dim() != 1 || heads.dim() != 1 ||
      features.size(0) != target.size(0) ||
      features.size(0) != heads.size(0) ||
      schedule.indices.sizes() != torch::IntArrayRef({kSteps, kBatchSize})) {
    phase2b_fail("Phase 2B training tensor contract mismatch");
  }
  require_finite_parameters_and_gradients(model, false);
  torch::optim::AdamOptions adam_options(kLearningRate);
  adam_options.betas(std::make_tuple(kAdamBeta1, kAdamBeta2));
  adam_options.eps(kAdamEpsilon);
  adam_options.weight_decay(kWeightDecay);
  torch::optim::Adam optimizer(model->parameters(), adam_options);

  TrainingSummary summary;
  model->train();
  for (int64_t step = 0; step < kSteps; ++step) {
    const auto batch = schedule.indices.select(0, step);
    const auto prediction = model->forward(features.index_select(0, batch),
                                           heads.index_select(0, batch));
    const auto loss = torch::mse_loss(prediction, target.index_select(0, batch));
    if (!torch::isfinite(loss).item<bool>()) {
      phase2b_fail("Phase 2B training produced a nonfinite loss");
    }
    optimizer.zero_grad();
    loss.backward();
    require_finite_parameters_and_gradients(model, true);
    const double gradient_norm =
        torch::nn::utils::clip_grad_norm_(model->parameters(),
                                         kGradClipNorm);
    if (!std::isfinite(gradient_norm)) {
      phase2b_fail("Phase 2B training produced a nonfinite gradient norm");
    }
    summary.maximum_gradient_norm =
        std::max(summary.maximum_gradient_norm, gradient_norm);
    optimizer.step();
    require_finite_parameters_and_gradients(model, false);
    summary.last_loss = loss.item<double>();
    ++summary.optimizer_steps;
  }
  if (summary.optimizer_steps != kSteps) {
    phase2b_fail("Phase 2B training did not complete its exact schedule");
  }
  return summary;
}

torch::Tensor predict_original_units(MatchedMlp &model,
                                     const torch::Tensor &features,
                                     const torch::Tensor &heads,
                                     int64_t anchor_count,
                                     const StandardizedTarget &target) {
  torch::NoGradGuard no_grad;
  model->eval();
  const auto standardized = model->forward(features, heads)
                                .reshape({anchor_count, kEdgeCount,
                                          kChannelCount})
                                .to(torch::kFloat64);
  const auto prediction =
      standardized * target.scale.view({1, kEdgeCount, kChannelCount}) +
      target.mean.view({1, kEdgeCount, kChannelCount});
  if (!torch::isfinite(prediction).all().item<bool>()) {
    phase2b_fail("Phase 2B evaluation produced a nonfinite prediction");
  }
  return prediction.contiguous();
}

SeedEvaluation evaluate_seed(MatchedMlp &model,
                             const StandardizedArm &arm,
                             const StandardizedTarget &target,
                             const torch::Tensor &train_heads,
                             const torch::Tensor &validation_heads,
                             const MatchedDataset &train_dataset,
                             const MatchedDataset &validation_dataset,
                             const BatchSchedule &schedule, int64_t seed) {
  SeedEvaluation result;
  result.seed = seed;
  result.training = train_once(model, arm.train, target.train, train_heads,
                               schedule);
  const auto train_prediction = predict_original_units(
      model, arm.train, train_heads, train_dataset.anchor_end -
                                         train_dataset.anchor_begin,
      target);
  const auto validation_prediction = predict_original_units(
      model, arm.validation, validation_heads,
      validation_dataset.anchor_end - validation_dataset.anchor_begin, target);
  result.train = summarize(observe(train_prediction, train_dataset.target));
  result.validation =
      summarize(observe(validation_prediction, validation_dataset.target));
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    result.train_channels[static_cast<std::size_t>(channel)] = summarize(
        observe(train_prediction.narrow(2, channel, 1),
                train_dataset.target.narrow(2, channel, 1)));
    result.validation_channels[static_cast<std::size_t>(channel)] = summarize(
        observe(validation_prediction.narrow(2, channel, 1),
                validation_dataset.target.narrow(2, channel, 1)));
  }
  result.validation_strong_gate_pass = strong_gate(result.validation);
  result.validation_partial_gate_pass = partial_gate(result.validation);
  return result;
}

double median_three(double a, double b, double c) {
  std::array<double, 3> values{a, b, c};
  std::sort(values.begin(), values.end());
  return values[1];
}

MetricSummary median_metric(
    const std::array<MetricSummary, kSeeds.size()> &metrics) {
  if (!(metrics[0].count == metrics[1].count &&
        metrics[1].count == metrics[2].count &&
        metrics[0].pair_count == metrics[1].pair_count &&
        metrics[1].pair_count == metrics[2].pair_count)) {
    phase2b_fail("Phase 2B median metrics have incompatible domains");
  }
  const auto median = [&](auto member) {
    return median_three(metrics[0].*member, metrics[1].*member,
                        metrics[2].*member);
  };
  return {.count = metrics[0].count,
          .pair_count = metrics[0].pair_count,
          .mae = median(&MetricSummary::mae),
          .rmse = median(&MetricSummary::rmse),
          .target_rms = median(&MetricSummary::target_rms),
          .prediction_rms = median(&MetricSummary::prediction_rms),
          .rmse_target_rms_ratio =
              median(&MetricSummary::rmse_target_rms_ratio),
          .direction = median(&MetricSummary::direction),
          .rank = median(&MetricSummary::rank),
          .best_asset = median(&MetricSummary::best_asset),
          .correlation = median(&MetricSummary::correlation)};
}

void finalize_arm(ArmEvaluation &arm) {
  std::array<MetricSummary, kSeeds.size()> train{};
  std::array<MetricSummary, kSeeds.size()> validation{};
  for (std::size_t index = 0; index < kSeeds.size(); ++index) {
    train[index] = arm.seeds[index].train;
    validation[index] = arm.seeds[index].validation;
    if (arm.seeds[index].validation_strong_gate_pass) {
      ++arm.strong_seed_count;
    }
  }
  arm.median_train = median_metric(train);
  arm.median_validation = median_metric(validation);
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    std::array<MetricSummary, kSeeds.size()> channel_train{};
    std::array<MetricSummary, kSeeds.size()> channel_validation{};
    for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
      channel_train[seed] =
          arm.seeds[seed].train_channels[static_cast<std::size_t>(channel)];
      channel_validation[seed] = arm.seeds[seed].validation_channels[
          static_cast<std::size_t>(channel)];
    }
    arm.median_train_channels[static_cast<std::size_t>(channel)] =
        median_metric(channel_train);
    arm.median_validation_channels[static_cast<std::size_t>(channel)] =
        median_metric(channel_validation);
  }
  arm.pass = arm.strong_seed_count >= 2;
}

std::string phase2b_classification(bool raw_pass, bool representation_pass) {
  if (raw_pass && representation_pass) {
    return "nonlinear_decodability_established";
  }
  if (raw_pass && !representation_pass) {
    return "information_not_established_at_frozen_raw96_interface";
  }
  if (!raw_pass && representation_pass) {
    return "representation_decodable_raw_history_control_invalid";
  }
  return "inconclusive_both_mlp_arms_failed";
}

void emit_seed(std::ostream &output, const std::string &arm_prefix,
               const SeedEvaluation &seed, const std::string &fingerprint) {
  const auto prefix = arm_prefix + ".seed_" + std::to_string(seed.seed);
  output << prefix << ".schedule_fingerprint=" << fingerprint << '\n';
  output << prefix << ".optimizer_steps=" << seed.training.optimizer_steps
         << '\n';
  output << prefix << ".last_loss=" << seed.training.last_loss << '\n';
  output << prefix << ".maximum_gradient_norm="
         << seed.training.maximum_gradient_norm << '\n';
  emit_metric(output, prefix + ".train", seed.train);
  emit_metric(output, prefix + ".validation", seed.validation);
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    emit_metric(output,
                prefix + ".train.channel_" + std::to_string(channel),
                seed.train_channels[static_cast<std::size_t>(channel)]);
    emit_metric(output,
                prefix + ".validation.channel_" + std::to_string(channel),
                seed.validation_channels[static_cast<std::size_t>(channel)]);
  }
  output << prefix << ".validation_strong_gate_pass="
         << (seed.validation_strong_gate_pass ? "true" : "false") << '\n';
  output << prefix << ".validation_partial_gate_pass="
         << (seed.validation_partial_gate_pass ? "true" : "false") << '\n';
}

void emit_arm(std::ostream &output, const ArmEvaluation &arm,
              const std::array<std::string, kSeeds.size()> &fingerprints) {
  const std::string prefix = "arm." + arm.name;
  for (std::size_t seed = 0; seed < kSeeds.size(); ++seed) {
    emit_seed(output, prefix, arm.seeds[seed], fingerprints[seed]);
  }
  emit_metric(output, prefix + ".median.train", arm.median_train);
  emit_metric(output, prefix + ".median.validation", arm.median_validation);
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    emit_metric(output,
                prefix + ".median.train.channel_" + std::to_string(channel),
                arm.median_train_channels[static_cast<std::size_t>(channel)]);
    emit_metric(
        output,
        prefix + ".median.validation.channel_" + std::to_string(channel),
        arm.median_validation_channels[static_cast<std::size_t>(channel)]);
  }
  output << prefix << ".strong_seed_count=" << arm.strong_seed_count << '\n';
  output << prefix << ".pass=" << (arm.pass ? "true" : "false") << '\n';
}

void write_phase2b_report_exclusive(const std::filesystem::path &path,
                                    const std::string &contents) {
  const int descriptor = ::open(path.c_str(),
                                O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
  if (descriptor < 0) {
    phase2b_fail("Phase 2B output must be absent and exclusively creatable: " +
                 std::string(std::strerror(errno)));
  }
  int open_descriptor = descriptor;
  try {
    std::size_t offset = 0;
    while (offset < contents.size()) {
      const auto written =
          ::write(open_descriptor, contents.data() + offset,
                  contents.size() - offset);
      if (written < 0 && errno == EINTR) {
        continue;
      }
      if (written <= 0) {
        phase2b_fail("failed while writing the exclusive Phase 2B report");
      }
      offset += static_cast<std::size_t>(written);
    }
    if (::close(open_descriptor) != 0) {
      open_descriptor = -1;
      phase2b_fail("failed while closing the exclusive Phase 2B report");
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

void run_phase2b(const Phase2BOptions &options) {
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
  at::globalContext().setDeterministicFillUninitializedMemory(true);

  const auto train_dataset = read_matched_probes(
      options.representation_train_input, options.raw_train_input, kTrainBegin,
      kTrainEnd);
  const auto validation_dataset = read_matched_probes(
      options.representation_validation_input, options.raw_validation_input,
      kValidationBegin, kValidationEnd);
  const auto raw_arm = standardize_arm(train_dataset.raw_features,
                                       validation_dataset.raw_features);
  const auto representation_arm = standardize_arm(
      train_dataset.representation_features,
      validation_dataset.representation_features);
  const auto target =
      standardize_target(train_dataset.target, validation_dataset.target);
  const auto train_heads =
      head_indices(train_dataset.anchor_end - train_dataset.anchor_begin);
  const auto validation_heads = head_indices(validation_dataset.anchor_end -
                                             validation_dataset.anchor_begin);

  ArmEvaluation raw_evaluation{.name = "raw_history_96"};
  ArmEvaluation representation_evaluation{.name = "representation_raw96"};
  std::array<std::string, kSeeds.size()> schedule_fingerprints{};
  int64_t fits_completed = 0;
  for (std::size_t index = 0; index < kSeeds.size(); ++index) {
    const int64_t seed = kSeeds[index];
    torch::manual_seed(seed);
    MatchedMlp raw_model;
    torch::manual_seed(seed);
    MatchedMlp representation_model;
    require_identical_initialization(raw_model, representation_model);
    const auto schedule = make_batch_schedule(seed, raw_arm.train.size(0));
    schedule_fingerprints[index] = schedule.fingerprint;
    raw_evaluation.seeds[index] = evaluate_seed(
        raw_model, raw_arm, target, train_heads, validation_heads,
        train_dataset, validation_dataset, schedule, seed);
    ++fits_completed;
    representation_evaluation.seeds[index] = evaluate_seed(
        representation_model, representation_arm, target, train_heads,
        validation_heads, train_dataset, validation_dataset, schedule, seed);
    ++fits_completed;
  }
  if (fits_completed != 6) {
    phase2b_fail("Phase 2B did not complete exactly six fits");
  }
  finalize_arm(raw_evaluation);
  finalize_arm(representation_evaluation);

  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::setprecision(17);
  output << "schema_id=" << kPhase2BSchema << '\n';
  output << "status=complete\n";
  output << "benchmark_id=synthetic_continuous_graph_v2\n";
  output << "diagnostic_phase=2B\n";
  output << "diagnostic_authority=development_only\n";
  output << "benchmark_acceptance_authority=false\n";
  output << "representation_record_schema="
         << kRepresentationSpec.record_schema << '\n';
  output << "raw_record_schema=" << kRawRecordSchema << '\n';
  output << "probe_header=" << kProbeHeader << '\n';
  output << "representation_train_input="
         << options.representation_train_input.string() << '\n';
  output << "representation_validation_input="
         << options.representation_validation_input.string() << '\n';
  output << "raw_train_input=" << options.raw_train_input.string() << '\n';
  output << "raw_validation_input=" << options.raw_validation_input.string()
         << '\n';
  output << "train_probe_rows=" << train_dataset.rows << '\n';
  output << "validation_probe_rows=" << validation_dataset.rows << '\n';
  output << "fit_anchor_range=[0,2496)\n";
  output << "validation_anchor_range=[2560,2816)\n";
  output << "certified_anchor_range=not_opened\n";
  output << "maximum_anchor_read=2815\n";
  output << "final_holdout_access=false\n";
  output << "policy_access=false\n";
  output << "representation_forward_executed=false\n";
  output << "checkpoint_written=false\n";
  output << "refit_after_evaluation=false\n";
  output << "coordinate_columns_exact_identity=true\n";
  output << "target_columns_exact_identity=true\n";
  output << "target_tensor_shared_between_arms=true\n";
  output << "row_order_exact_identity=true\n";
  output << "feature_layout=base_32,quote_32,base_minus_quote_32\n";
  output << "feature_width=96\n";
  output << "representation_feature_identity_max_abs_delta="
         << std::max(train_dataset.representation_identity_max_abs_delta,
                     validation_dataset.representation_identity_max_abs_delta)
         << '\n';
  output << "raw_feature_identity_max_abs_delta="
         << std::max(train_dataset.raw_identity_max_abs_delta,
                     validation_dataset.raw_identity_max_abs_delta)
         << '\n';
  output << "raw_configured_capacity_leading_zero_prefix_verified=true\n";
  output << "raw_configured_capacity_padding_zero_values_checked="
         << train_dataset
                    .raw_configured_capacity_padding_zero_values_checked +
                validation_dataset
                    .raw_configured_capacity_padding_zero_values_checked
         << '\n';
  output << "raw_configured_capacities=4,10,30\n";
  output << "raw_values_within_configured_capacity_may_be_zero=true\n";
  output << "input_standardization=train_global_per_arm\n";
  output << "input_standard_deviation_floor=" << kStandardDeviationFloor
         << '\n';
  output << "raw_input_clamped_coordinate_count="
         << raw_arm.clamped_coordinate_count << '\n';
  output << "representation_input_clamped_coordinate_count="
         << representation_arm.clamped_coordinate_count << '\n';
  output << "target_standardization=train_per_edge_channel\n";
  output << "target_standard_deviation_floor=" << kStandardDeviationFloor
         << '\n';
  output << "target_clamped_coordinate_count="
         << target.clamped_coordinate_count << '\n';
  output << "architecture=linear_96_128_gelu_128_128_gelu_128_9\n";
  output << "head_selection=channel_index_times_3_plus_edge_index\n";
  output << "dropout=0\n";
  output << "normalization_layers=false\n";
  output << "residual_branch=false\n";
  output << "mixture_distribution=false\n";
  output << "loss=standardized_target_mean_squared_error\n";
  output << "device=cpu\n";
  output << "dtype=float32\n";
  output << "deterministic_algorithms=true\n";
  output << "seeds=31,47,73\n";
  output << "steps_per_fit=" << kSteps << '\n';
  output << "batch_size=" << kBatchSize << '\n';
  output << "optimizer=adam\n";
  output << "learning_rate=" << kLearningRate << '\n';
  output << "adam_beta1=" << kAdamBeta1 << '\n';
  output << "adam_beta2=" << kAdamBeta2 << '\n';
  output << "adam_epsilon=" << kAdamEpsilon << '\n';
  output << "weight_decay=" << kWeightDecay << '\n';
  output << "gradient_clip_norm=" << kGradClipNorm << '\n';
  output << "batch_sampling=mt19937_64_uniform_with_replacement\n";
  output << "paired_initial_parameters_exact=true\n";
  output << "paired_batch_schedule_exact=true\n";
  output << "validation_read_by_trainer=false\n";
  output << "early_stopping=false\n";
  output << "seed_selection=false\n";
  output << "hyperparameter_search=false\n";
  output << "retry=false\n";
  output << "fits_completed=" << fits_completed << '\n';
  output << "preregistered_strong_gate=direction>=0.95,rank>=0.95,"
            "correlation>=0.95,rmse_target_rms_ratio<=0.25\n";
  output << "arm_pass_rule=validation_strong_gate_in_at_least_2_of_3_seeds\n";
  emit_arm(output, raw_evaluation, schedule_fingerprints);
  emit_arm(output, representation_evaluation, schedule_fingerprints);
  output << "classification="
         << phase2b_classification(raw_evaluation.pass,
                                   representation_evaluation.pass)
         << '\n';
  if (!output) {
    phase2b_fail("failed while rendering the Phase 2B report");
  }
  write_phase2b_report_exclusive(options.output, output.str());
}

} // namespace

int main(int argc, char **argv) {
  try {
    std::locale::global(std::locale::classic());
    run_phase2b(parse_phase2b_options(argc, argv));
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "matched nonlinear sufficiency corrected-control probe: "
              << error.what() << '\n';
    return 1;
  }
}
