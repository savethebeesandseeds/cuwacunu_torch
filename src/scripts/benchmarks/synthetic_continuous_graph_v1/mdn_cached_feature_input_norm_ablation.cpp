// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <ATen/Context.h>
#include <torch/torch.h>

#include "piaabo/digest/sha256.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network_head.h"

namespace {

namespace digest = cuwacunu::piaabo::digest;
namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;

constexpr int64_t kAnchorCount = 730;
constexpr int64_t kEdgeCount = 3;
constexpr int64_t kChannelCount = 3;
constexpr int64_t kHiddenWidth = 128;
constexpr int64_t kIdentityEmbeddingWidth = 16;
constexpr int64_t kDynamicFeatureCount = 3 * kHiddenWidth;
constexpr int64_t kReadoutFeatureCount =
    kDynamicFeatureCount + kIdentityEmbeddingWidth;
constexpr int64_t kExpectedRows = kAnchorCount * kEdgeCount * kChannelCount;
constexpr int64_t kFitBegin = 0;
constexpr int64_t kFitEnd = 554;
constexpr int64_t kPurgeEnd = 584;
constexpr int64_t kValidationEnd = 730;
constexpr int64_t kCanaryBegin = 0;
constexpr int64_t kCanaryEnd = 16;
constexpr int64_t kTrainingSteps = 3500;
constexpr int64_t kCanarySteps = 1000;
constexpr int64_t kBatchSize = 64;
constexpr int64_t kSeed = 31;
constexpr double kLearningRate = 1.0e-3;
constexpr double kGradClipNorm = 5.0;
constexpr double kRegressionWeight = 100.0;
constexpr double kDirectionWeight = 5.0;
constexpr double kRankWeight = 5.0;
constexpr double kHuberBeta = 0.5;
constexpr double kLogitScale = 1.0;
constexpr double kTargetScale = 36.0;
constexpr double kMarginEps = 1.0e-3;

enum class NormalizationMode { kSampleLayerNorm, kNone };

struct Options {
  std::filesystem::path input;
  std::filesystem::path head_header;
  std::filesystem::path output;
  std::string schema_id{"synthetic_mdn_cached_feature_input_norm_ablation_v1"};
  bool force_cpu{false};
};

[[noreturn]] void fail(const std::string &message) {
  throw std::runtime_error(message);
}

void require(bool condition, const std::string &message) {
  if (!condition) {
    fail(message);
  }
}

std::string required_value(int argc, char **argv, int &index,
                           const std::string &flag) {
  require(index + 1 < argc, "missing value for " + flag);
  return argv[++index];
}

Options parse_options(int argc, char **argv) {
  Options options{};
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--input") {
      options.input = required_value(argc, argv, index, argument);
    } else if (argument == "--head-header") {
      options.head_header = required_value(argc, argv, index, argument);
    } else if (argument == "--output") {
      options.output = required_value(argc, argv, index, argument);
    } else if (argument == "--schema-id") {
      options.schema_id = required_value(argc, argv, index, argument);
    } else if (argument == "--cpu") {
      options.force_cpu = true;
    } else {
      fail("unknown argument: " + argument);
    }
  }
  require(!options.input.empty(), "--input is required");
  require(!options.head_header.empty(), "--head-header is required");
  require(!options.output.empty(), "--output is required");
  require(options.schema_id ==
              "synthetic_mdn_cached_feature_input_norm_ablation_v1",
          "unexpected schema id");
  return options;
}

std::string read_file(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  require(input.good(), "cannot open input: " + path.string());
  std::ostringstream buffer;
  buffer << input.rdbuf();
  require(input.good() || input.eof(), "cannot read input: " + path.string());
  return buffer.str();
}

std::string sha256_file(const std::filesystem::path &path) {
  return digest::sha256_hex(read_file(path));
}

int64_t parse_int64(const std::string &token, int64_t row,
                    const char *field_name) {
  std::size_t consumed = 0;
  const int64_t value = std::stoll(token, &consumed);
  require(consumed == token.size(), std::string("invalid ") + field_name +
                                        " at row " + std::to_string(row));
  return value;
}

double parse_double(const std::string &token, int64_t row,
                    const char *field_name) {
  std::size_t consumed = 0;
  const double value = std::stod(token, &consumed);
  require(consumed == token.size() && std::isfinite(value),
          std::string("invalid ") + field_name + " at row " +
              std::to_string(row));
  return value;
}

std::array<std::string, 12> split_probe_row(const std::string &line,
                                            int64_t row) {
  std::array<std::string, 12> fields{};
  std::istringstream stream(line);
  for (auto &field : fields) {
    require(static_cast<bool>(std::getline(stream, field, ',')),
            "probe row has too few fields at row " + std::to_string(row));
  }
  std::string trailing;
  require(!std::getline(stream, trailing, ','),
          "probe row has too many fields at row " + std::to_string(row));
  return fields;
}

std::array<float, kReadoutFeatureCount>
parse_features(const std::string &serialized, int64_t row) {
  std::array<float, kReadoutFeatureCount> values{};
  std::istringstream stream(serialized);
  for (int64_t feature = 0; feature < kReadoutFeatureCount; ++feature) {
    std::string token;
    require(static_cast<bool>(std::getline(stream, token, ';')),
            "probe feature vector is short at row " + std::to_string(row));
    values[static_cast<std::size_t>(feature)] =
        static_cast<float>(parse_double(token, row, "feature_values"));
  }
  std::string trailing;
  require(!std::getline(stream, trailing, ';'),
          "probe feature vector is long at row " + std::to_string(row));
  return values;
}

struct Dataset {
  torch::Tensor features; // [A,E,C,400], exact cached post-adapter readout.
  torch::Tensor target;   // [A,E,C], captured next close edge return.
  std::string input_sha256;
  double dynamic_difference_max_abs_error{0.0};
  double quote_consistency_max_abs_error{0.0};
  double identity_consistency_max_abs_error{0.0};
  double identity_min_pair_distance{std::numeric_limits<double>::infinity()};
};

Dataset read_dataset(const Options &options) {
  constexpr const char *kHeader =
      "record_schema,anchor_key,anchor_index,anchor_local_index,edge_index,"
      "edge_id,base_node_id,quote_node_id,channel_index,"
      "target_edge_close_return,feature_count,feature_values";
  constexpr const char *kRecordSchema =
      "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1";
  const std::array<std::string, kEdgeCount> edge_ids{
      "SYNALPHASYNUSD", "SYNBETASYNUSD", "SYNGAMMASYNUSD"};
  const std::array<std::string, kEdgeCount> base_ids{"SYNALPHA", "SYNBETA",
                                                     "SYNGAMMA"};

  Dataset dataset{};
  dataset.input_sha256 = sha256_file(options.input);
  dataset.features = torch::empty(
      {kAnchorCount, kEdgeCount, kChannelCount, kReadoutFeatureCount},
      torch::kFloat32);
  dataset.target =
      torch::empty({kAnchorCount, kEdgeCount, kChannelCount}, torch::kFloat32);
  auto features = dataset.features.accessor<float, 4>();
  auto target = dataset.target.accessor<float, 3>();

  std::array<std::array<std::array<float, kHiddenWidth>, kChannelCount>,
             kAnchorCount>
      quote_reference{};
  std::array<std::array<bool, kChannelCount>, kAnchorCount>
      quote_reference_seen{};
  std::array<std::array<float, kIdentityEmbeddingWidth>, kEdgeCount>
      identity_reference{};
  std::array<bool, kEdgeCount> identity_reference_seen{};
  std::array<int64_t, kAnchorCount> anchor_key_reference{};
  std::array<bool, kAnchorCount> anchor_key_reference_seen{};

  std::ifstream input(options.input);
  require(input.good(), "cannot open cached feature probe");
  std::string line;
  require(static_cast<bool>(std::getline(input, line)) && line == kHeader,
          "cached feature probe header mismatch");
  int64_t ordinal = 0;
  while (std::getline(input, line)) {
    require(ordinal < kExpectedRows, "cached feature probe has extra rows");
    const auto fields = split_probe_row(line, ordinal);
    const int64_t anchor = ordinal / (kEdgeCount * kChannelCount);
    const int64_t within = ordinal % (kEdgeCount * kChannelCount);
    const int64_t edge = within / kChannelCount;
    const int64_t channel = within % kChannelCount;
    const int64_t anchor_key = parse_int64(fields[1], ordinal, "anchor_key");
    require(fields[0] == kRecordSchema,
            "cached feature record schema mismatch");
    require(parse_int64(fields[2], ordinal, "anchor_index") == anchor &&
                parse_int64(fields[3], ordinal, "anchor_local_index") ==
                    anchor % kBatchSize &&
                parse_int64(fields[4], ordinal, "edge_index") == edge &&
                parse_int64(fields[8], ordinal, "channel_index") == channel,
            "cached feature row order or coordinate mismatch");
    if (!anchor_key_reference_seen[static_cast<std::size_t>(anchor)]) {
      anchor_key_reference[static_cast<std::size_t>(anchor)] = anchor_key;
      anchor_key_reference_seen[static_cast<std::size_t>(anchor)] = true;
    } else {
      require(anchor_key_reference[static_cast<std::size_t>(anchor)] ==
                  anchor_key,
              "cached feature anchor key differs within anchor group");
    }
    require(fields[5] == edge_ids[static_cast<std::size_t>(edge)] &&
                fields[6] == base_ids[static_cast<std::size_t>(edge)] &&
                fields[7] == "SYNUSD",
            "cached feature graph identity mismatch");
    require(parse_int64(fields[10], ordinal, "feature_count") ==
                kReadoutFeatureCount,
            "cached feature width mismatch");
    target[anchor][edge][channel] = static_cast<float>(
        parse_double(fields[9], ordinal, "target_edge_close_return"));
    const auto row_features = parse_features(fields[11], ordinal);
    for (int64_t feature = 0; feature < kReadoutFeatureCount; ++feature) {
      features[anchor][edge][channel][feature] =
          row_features[static_cast<std::size_t>(feature)];
    }
    for (int64_t feature = 0; feature < kHiddenWidth; ++feature) {
      const double base = row_features[static_cast<std::size_t>(feature)];
      const double quote =
          row_features[static_cast<std::size_t>(kHiddenWidth + feature)];
      const double difference =
          row_features[static_cast<std::size_t>(2 * kHiddenWidth + feature)];
      dataset.dynamic_difference_max_abs_error =
          std::max(dataset.dynamic_difference_max_abs_error,
                   std::abs((base - quote) - difference));
      if (!quote_reference_seen[static_cast<std::size_t>(anchor)]
                               [static_cast<std::size_t>(channel)]) {
        quote_reference[static_cast<std::size_t>(anchor)]
                       [static_cast<std::size_t>(channel)]
                       [static_cast<std::size_t>(feature)] =
                           static_cast<float>(quote);
      } else {
        dataset.quote_consistency_max_abs_error = std::max(
            dataset.quote_consistency_max_abs_error,
            std::abs(quote -
                     quote_reference[static_cast<std::size_t>(anchor)]
                                    [static_cast<std::size_t>(channel)]
                                    [static_cast<std::size_t>(feature)]));
      }
    }
    quote_reference_seen[static_cast<std::size_t>(anchor)]
                        [static_cast<std::size_t>(channel)] = true;
    for (int64_t identity = 0; identity < kIdentityEmbeddingWidth; ++identity) {
      const double value = row_features[static_cast<std::size_t>(
          kDynamicFeatureCount + identity)];
      if (!identity_reference_seen[static_cast<std::size_t>(edge)]) {
        identity_reference[static_cast<std::size_t>(edge)]
                          [static_cast<std::size_t>(identity)] =
                              static_cast<float>(value);
      } else {
        dataset.identity_consistency_max_abs_error = std::max(
            dataset.identity_consistency_max_abs_error,
            std::abs(value - identity_reference[static_cast<std::size_t>(
                                 edge)][static_cast<std::size_t>(identity)]));
      }
    }
    identity_reference_seen[static_cast<std::size_t>(edge)] = true;
    ++ordinal;
  }
  require(input.good() || input.eof(), "cannot read cached feature probe");
  require(ordinal == kExpectedRows, "cached feature probe row count mismatch");
  for (int64_t anchor = 1; anchor < kAnchorCount; ++anchor) {
    require(anchor_key_reference_seen[static_cast<std::size_t>(anchor)] &&
                anchor_key_reference[static_cast<std::size_t>(anchor)] >
                    anchor_key_reference[static_cast<std::size_t>(anchor - 1)],
            "cached feature anchor keys are incomplete or non-monotonic");
  }
  require(torch::isfinite(dataset.features).all().item<bool>() &&
              torch::isfinite(dataset.target).all().item<bool>(),
          "cached feature probe contains non-finite tensors");
  require(dataset.dynamic_difference_max_abs_error <= 1.0e-5,
          "cached base-minus-quote identity failed");
  require(dataset.quote_consistency_max_abs_error <= 1.0e-6,
          "cached quote identity differs across edges");
  require(dataset.identity_consistency_max_abs_error <= 1.0e-7,
          "cached edge embedding differs across anchors or channels");
  for (int64_t lhs = 0; lhs < kEdgeCount; ++lhs) {
    for (int64_t rhs = lhs + 1; rhs < kEdgeCount; ++rhs) {
      double squared_distance = 0.0;
      for (int64_t identity = 0; identity < kIdentityEmbeddingWidth;
           ++identity) {
        const double delta =
            identity_reference[static_cast<std::size_t>(lhs)]
                              [static_cast<std::size_t>(identity)] -
            identity_reference[static_cast<std::size_t>(rhs)]
                              [static_cast<std::size_t>(identity)];
        squared_distance += delta * delta;
      }
      dataset.identity_min_pair_distance = std::min(
          dataset.identity_min_pair_distance, std::sqrt(squared_distance));
    }
  }
  require(dataset.identity_min_pair_distance > 0.0,
          "cached edge identity embeddings are not distinct");
  return dataset;
}

std::vector<int64_t> range_indices(int64_t begin, int64_t end) {
  require(begin >= 0 && end >= begin && end <= kAnchorCount,
          "invalid anchor range");
  std::vector<int64_t> indices(static_cast<std::size_t>(end - begin));
  std::iota(indices.begin(), indices.end(), begin);
  return indices;
}

torch::Tensor index_tensor(const std::vector<int64_t> &indices,
                           const torch::Device &device) {
  return torch::tensor(indices, torch::TensorOptions().dtype(torch::kInt64))
      .to(device);
}

using BatchSchedule = std::vector<std::vector<int64_t>>;

BatchSchedule make_schedule(const std::vector<int64_t> &indices, int64_t steps,
                            int64_t seed) {
  require(!indices.empty() && steps > 0, "invalid training schedule request");
  std::mt19937_64 rng(static_cast<std::mt19937_64::result_type>(seed));
  std::vector<int64_t> order = indices;
  BatchSchedule schedule;
  schedule.reserve(static_cast<std::size_t>(steps));
  while (static_cast<int64_t>(schedule.size()) < steps) {
    std::shuffle(order.begin(), order.end(), rng);
    for (std::size_t offset = 0;
         offset < order.size() && static_cast<int64_t>(schedule.size()) < steps;
         offset += static_cast<std::size_t>(kBatchSize)) {
      const std::size_t end =
          std::min(order.size(), offset + static_cast<std::size_t>(kBatchSize));
      schedule.emplace_back(order.begin() + static_cast<std::ptrdiff_t>(offset),
                            order.begin() + static_cast<std::ptrdiff_t>(end));
    }
  }
  require(static_cast<int64_t>(schedule.size()) == steps,
          "training schedule length mismatch");
  return schedule;
}

std::string schedule_digest(const BatchSchedule &schedule) {
  std::ostringstream serialized;
  for (const auto &batch : schedule) {
    for (const int64_t anchor : batch) {
      serialized << anchor << ',';
    }
    serialized << ';';
  }
  return digest::sha256_hex(serialized.str());
}

void seed_torch(const torch::Device &device) {
  torch::manual_seed(kSeed);
  if (device.is_cuda()) {
    torch::cuda::manual_seed_all(kSeed);
  }
}

mdn::DirectEdgeReturnHead make_head(const torch::Device &device) {
  seed_torch(device);
  auto head = mdn::DirectEdgeReturnHead(mdn::DirectEdgeReturnHeadOptions{
      .feature_dim = kHiddenWidth,
      .quote_node_index = 0,
      .identity_mode = "edge_embedding_per_edge",
      .base_edge_count = kEdgeCount,
      .identity_embedding_dim = kIdentityEmbeddingWidth,
      // The cache is already after the trained Runtime adapter.
      .adapter_hidden_dim = 0,
  });
  head->to(device);
  return head;
}

std::string canonical_parameter_digest(const mdn::DirectEdgeReturnHead &head) {
  std::string material;
  for (const auto &entry : head->named_parameters(true)) {
    const auto tensor = entry.value().detach().to(torch::kCPU).contiguous();
    material.append(entry.key());
    material.push_back('\0');
    for (const auto dimension : tensor.sizes()) {
      material.append(std::to_string(dimension));
      material.push_back(',');
    }
    material.push_back('\0');
    const auto byte_count = static_cast<std::size_t>(
        tensor.numel() * static_cast<int64_t>(tensor.element_size()));
    material.append(reinterpret_cast<const char *>(tensor.data_ptr()),
                    byte_count);
  }
  return digest::sha256_hex(material);
}

void require_identical_parameters(const mdn::DirectEdgeReturnHead &lhs,
                                  const mdn::DirectEdgeReturnHead &rhs) {
  const auto left = lhs->named_parameters(true);
  const auto right = rhs->named_parameters(true);
  require(left.size() == right.size(), "A/B parameter count mismatch");
  bool saw_input_norm_weight = false;
  bool saw_input_norm_bias = false;
  for (std::size_t index = 0; index < left.size(); ++index) {
    require(left[index].key() == right[index].key(),
            "A/B parameter name order mismatch");
    require(torch::equal(left[index].value(), right[index].value()),
            "A/B initial parameter bytes differ: " + left[index].key());
    saw_input_norm_weight |= left[index].key() == "input_norm.weight";
    saw_input_norm_bias |= left[index].key() == "input_norm.bias";
  }
  require(saw_input_norm_weight && saw_input_norm_bias,
          "input_norm parameters were not retained in both arms");
  require(canonical_parameter_digest(lhs) == canonical_parameter_digest(rhs),
          "A/B canonical initial parameter digests differ");
}

torch::Tensor forward_manual(mdn::DirectEdgeReturnHead &head,
                             const torch::Tensor &readout_features,
                             bool apply_input_norm) {
  require(readout_features.sizes() ==
              torch::IntArrayRef({readout_features.size(0), kEdgeCount,
                                  kChannelCount, kReadoutFeatureCount}),
          "manual readout feature shape mismatch");
  const auto batch = readout_features.size(0);
  auto input = readout_features.contiguous().view(
      {batch * kEdgeCount * kChannelCount, kReadoutFeatureCount});
  if (apply_input_norm) {
    input = head->input_norm->forward(input);
  }
  auto hidden = torch::silu(head->hidden->forward(input))
                    .view({batch, kEdgeCount, kChannelCount, kHiddenWidth});
  using torch::indexing::Slice;
  std::vector<torch::Tensor> outputs;
  outputs.reserve(kEdgeCount);
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    auto edge_hidden = hidden.index({Slice(), edge, Slice(), Slice()})
                           .contiguous()
                           .view({batch * kChannelCount, kHiddenWidth});
    outputs.push_back(head->edge_projections[static_cast<std::size_t>(edge)]
                          ->forward(edge_hidden)
                          .view({batch, 1, kChannelCount}));
  }
  const auto output = torch::cat(outputs, 1);
  require(torch::isfinite(output).all().item<bool>(),
          "manual direct readout produced non-finite values");
  return output;
}

torch::Tensor forward_arm(mdn::DirectEdgeReturnHead &head,
                          const torch::Tensor &readout_features,
                          NormalizationMode mode) {
  if (mode == NormalizationMode::kSampleLayerNorm) {
    return head->forward_readout_features(readout_features);
  }
  return forward_manual(head, readout_features, false);
}

double
production_manual_parity_max_abs_error(mdn::DirectEdgeReturnHead &head,
                                       const torch::Tensor &readout_features) {
  torch::NoGradGuard no_grad;
  const auto production = head->forward_readout_features(readout_features);
  const auto manual = forward_manual(head, readout_features, true);
  return (production - manual).abs().max().to(torch::kCPU).item<double>();
}

struct Objective {
  torch::Tensor total;
  torch::Tensor regression;
  torch::Tensor direction;
  torch::Tensor rank;
};

Objective compute_objective(const torch::Tensor &prediction,
                            const torch::Tensor &target) {
  require(prediction.sizes() == target.sizes(),
          "prediction/target objective shape mismatch");
  const auto predicted_scaled = prediction * kTargetScale;
  const auto target_scaled = target * kTargetScale;
  const auto difference = predicted_scaled - target_scaled;
  const auto absolute_difference = difference.abs();
  const auto regression = torch::where(absolute_difference < kHuberBeta,
                                       0.5 * difference.pow(2) / kHuberBeta,
                                       absolute_difference - 0.5 * kHuberBeta)
                              .mean();

  const auto sign = target.sign();
  const auto active = sign.ne(0);
  require(active.sum().item<int64_t>() > 0,
          "direction objective has no active targets");
  const auto direction =
      torch::softplus(-(predicted_scaled.masked_select(active) *
                        sign.masked_select(active) * kLogitScale))
          .mean();

  auto rank_sum = prediction.new_zeros({});
  int64_t rank_count = 0;
  for (int64_t lhs = 0; lhs < kEdgeCount; ++lhs) {
    for (int64_t rhs = lhs + 1; rhs < kEdgeCount; ++rhs) {
      const auto predicted_difference =
          (prediction.select(1, lhs) - prediction.select(1, rhs)) *
          kTargetScale;
      const auto rank_sign =
          (target.select(1, lhs) - target.select(1, rhs)).sign();
      const auto rank_active = rank_sign.ne(0);
      const auto active_count = rank_active.sum().item<int64_t>();
      if (active_count == 0) {
        continue;
      }
      rank_sum =
          rank_sum +
          torch::softplus(-(predicted_difference.masked_select(rank_active) *
                            rank_sign.masked_select(rank_active) * kLogitScale))
              .sum();
      rank_count += active_count;
    }
  }
  require(rank_count > 0, "rank objective has no active pairs");
  const auto rank = rank_sum / static_cast<double>(rank_count);
  const auto total = kRegressionWeight * regression +
                     kDirectionWeight * direction + kRankWeight * rank;
  return {total, regression, direction, rank};
}

int sign_of(double value) { return value > 0.0 ? 1 : (value < 0.0 ? -1 : 0); }

struct Metric {
  int64_t count{0};
  int64_t direction_correct{0};
  int64_t margin_count{0};
  int64_t margin_direction_correct{0};
  int64_t pair_count{0};
  int64_t pair_correct{0};
  int64_t margin_pair_count{0};
  int64_t margin_pair_correct{0};
  int64_t best_count{0};
  int64_t best_correct{0};
  int64_t near_zero_target_count{0};
  std::array<int64_t, kEdgeCount> edge_count{};
  std::array<int64_t, kEdgeCount> edge_direction_correct{};
  std::array<int64_t, kChannelCount> channel_count{};
  std::array<int64_t, kChannelCount> channel_direction_correct{};
  double error_sum{0.0};
  double absolute_error_sum{0.0};
  double squared_error_sum{0.0};
  double prediction_sum{0.0};
  double target_sum{0.0};
  double prediction_squared_sum{0.0};
  double target_squared_sum{0.0};
  double cross_sum{0.0};
  double prediction_min{std::numeric_limits<double>::infinity()};
  double prediction_max{-std::numeric_limits<double>::infinity()};
  double target_min{std::numeric_limits<double>::infinity()};
  double target_max{-std::numeric_limits<double>::infinity()};

  double ratio(int64_t numerator, int64_t denominator) const {
    return denominator > 0 ? static_cast<double>(numerator) /
                                 static_cast<double>(denominator)
                           : std::numeric_limits<double>::quiet_NaN();
  }
  double mae() const { return count > 0 ? absolute_error_sum / count : 0.0; }
  double rmse() const {
    return count > 0 ? std::sqrt(squared_error_sum / count) : 0.0;
  }
  double directional_accuracy() const {
    return ratio(direction_correct, count);
  }
  double margin_directional_accuracy() const {
    return ratio(margin_direction_correct, margin_count);
  }
  double rank_accuracy() const { return ratio(pair_correct, pair_count); }
  double margin_rank_accuracy() const {
    return ratio(margin_pair_correct, margin_pair_count);
  }
  double best_agreement() const { return ratio(best_correct, best_count); }
  double prediction_std() const {
    if (count == 0) {
      return 0.0;
    }
    const double mean = prediction_sum / count;
    return std::sqrt(
        std::max(0.0, prediction_squared_sum / count - mean * mean));
  }
  double target_std() const {
    if (count == 0) {
      return 0.0;
    }
    const double mean = target_sum / count;
    return std::sqrt(std::max(0.0, target_squared_sum / count - mean * mean));
  }
  double correlation() const {
    if (count <= 1) {
      return 0.0;
    }
    const double n = static_cast<double>(count);
    const double covariance = cross_sum - prediction_sum * target_sum / n;
    const double prediction_variance =
        prediction_squared_sum - prediction_sum * prediction_sum / n;
    const double target_variance =
        target_squared_sum - target_sum * target_sum / n;
    if (prediction_variance <= 0.0 || target_variance <= 0.0) {
      return 0.0;
    }
    return covariance / std::sqrt(prediction_variance * target_variance);
  }
};

void observe_metric(Metric &metric, const torch::Tensor &prediction,
                    const torch::Tensor &target) {
  const auto predicted = prediction.to(torch::kCPU).contiguous();
  const auto realized = target.to(torch::kCPU).contiguous();
  const auto p = predicted.accessor<float, 3>();
  const auto y = realized.accessor<float, 3>();
  for (int64_t batch = 0; batch < predicted.size(0); ++batch) {
    for (int64_t channel = 0; channel < kChannelCount; ++channel) {
      std::array<double, kEdgeCount> prediction_by_edge{};
      std::array<double, kEdgeCount> target_by_edge{};
      for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
        const double prediction_value = p[batch][edge][channel];
        const double target_value = y[batch][edge][channel];
        require(std::isfinite(prediction_value) && std::isfinite(target_value),
                "metric received non-finite values");
        prediction_by_edge[static_cast<std::size_t>(edge)] = prediction_value;
        target_by_edge[static_cast<std::size_t>(edge)] = target_value;
        const double error = prediction_value - target_value;
        ++metric.count;
        ++metric.edge_count[static_cast<std::size_t>(edge)];
        ++metric.channel_count[static_cast<std::size_t>(channel)];
        metric.error_sum += error;
        metric.absolute_error_sum += std::abs(error);
        metric.squared_error_sum += error * error;
        metric.prediction_sum += prediction_value;
        metric.target_sum += target_value;
        metric.prediction_squared_sum += prediction_value * prediction_value;
        metric.target_squared_sum += target_value * target_value;
        metric.cross_sum += prediction_value * target_value;
        metric.prediction_min =
            std::min(metric.prediction_min, prediction_value);
        metric.prediction_max =
            std::max(metric.prediction_max, prediction_value);
        metric.target_min = std::min(metric.target_min, target_value);
        metric.target_max = std::max(metric.target_max, target_value);
        const bool direction_match =
            sign_of(prediction_value) == sign_of(target_value);
        if (direction_match) {
          ++metric.direction_correct;
          ++metric.edge_direction_correct[static_cast<std::size_t>(edge)];
          ++metric.channel_direction_correct[static_cast<std::size_t>(channel)];
        }
        if (std::abs(target_value) > kMarginEps) {
          ++metric.margin_count;
          if (direction_match) {
            ++metric.margin_direction_correct;
          }
        } else {
          ++metric.near_zero_target_count;
        }
      }
      for (int64_t lhs = 0; lhs < kEdgeCount; ++lhs) {
        for (int64_t rhs = lhs + 1; rhs < kEdgeCount; ++rhs) {
          const double prediction_difference =
              prediction_by_edge[static_cast<std::size_t>(lhs)] -
              prediction_by_edge[static_cast<std::size_t>(rhs)];
          const double target_difference =
              target_by_edge[static_cast<std::size_t>(lhs)] -
              target_by_edge[static_cast<std::size_t>(rhs)];
          ++metric.pair_count;
          if (sign_of(prediction_difference) == sign_of(target_difference)) {
            ++metric.pair_correct;
          }
          if (std::abs(target_difference) > kMarginEps) {
            ++metric.margin_pair_count;
            if (sign_of(prediction_difference) == sign_of(target_difference)) {
              ++metric.margin_pair_correct;
            }
          }
        }
      }
      const auto predicted_best = static_cast<int64_t>(
          std::distance(prediction_by_edge.begin(),
                        std::max_element(prediction_by_edge.begin(),
                                         prediction_by_edge.end())));
      const auto target_best = static_cast<int64_t>(std::distance(
          target_by_edge.begin(),
          std::max_element(target_by_edge.begin(), target_by_edge.end())));
      ++metric.best_count;
      if (predicted_best == target_best) {
        ++metric.best_correct;
      }
    }
  }
}

struct Evaluation {
  Metric metric{};
  double objective_total{0.0};
  double objective_regression{0.0};
  double objective_direction{0.0};
  double objective_rank{0.0};
  bool finite{false};
};

Evaluation evaluate(mdn::DirectEdgeReturnHead &head, NormalizationMode mode,
                    const torch::Tensor &features, const torch::Tensor &target,
                    const std::vector<int64_t> &indices) {
  require(!indices.empty(), "cannot evaluate an empty split");
  head->eval();
  torch::NoGradGuard no_grad;
  const auto selected = index_tensor(indices, features.device());
  const auto selected_features = features.index_select(0, selected);
  const auto selected_target = target.index_select(0, selected);
  const auto prediction = forward_arm(head, selected_features, mode);
  const auto objective = compute_objective(prediction, selected_target);
  Evaluation evaluation{};
  observe_metric(evaluation.metric, prediction, selected_target);
  evaluation.objective_total = objective.total.to(torch::kCPU).item<double>();
  evaluation.objective_regression =
      objective.regression.to(torch::kCPU).item<double>();
  evaluation.objective_direction =
      objective.direction.to(torch::kCPU).item<double>();
  evaluation.objective_rank = objective.rank.to(torch::kCPU).item<double>();
  evaluation.finite = std::isfinite(evaluation.objective_total) &&
                      std::isfinite(evaluation.objective_regression) &&
                      std::isfinite(evaluation.objective_direction) &&
                      std::isfinite(evaluation.objective_rank);
  return evaluation;
}

using ParameterSnapshot = std::vector<std::pair<std::string, torch::Tensor>>;

ParameterSnapshot snapshot_parameters(const mdn::DirectEdgeReturnHead &head) {
  ParameterSnapshot snapshot;
  for (const auto &entry : head->named_parameters(true)) {
    snapshot.emplace_back(entry.key(), entry.value().detach().clone());
  }
  return snapshot;
}

double parameter_prefix_max_abs_delta(const mdn::DirectEdgeReturnHead &head,
                                      const ParameterSnapshot &before,
                                      const std::string &prefix) {
  const auto parameters = head->named_parameters(true);
  require(parameters.size() == before.size(),
          "parameter count changed during training");
  double maximum = 0.0;
  bool found = false;
  for (std::size_t index = 0; index < before.size(); ++index) {
    require(parameters[index].key() == before[index].first,
            "parameter order changed during training");
    if (before[index].first.rfind(prefix, 0) != 0) {
      continue;
    }
    found = true;
    maximum = std::max(
        maximum, (parameters[index].value().detach() - before[index].second)
                     .abs()
                     .max()
                     .to(torch::kCPU)
                     .item<double>());
  }
  require(found, "parameter prefix not found: " + prefix);
  return maximum;
}

struct TrainSummary {
  double first_objective{0.0};
  double final_objective{0.0};
  double input_norm_parameter_max_abs_delta{0.0};
  double edge_embedding_parameter_max_abs_delta{0.0};
  int64_t optimizer_steps{0};
};

TrainSummary train_head(mdn::DirectEdgeReturnHead &head, NormalizationMode mode,
                        const torch::Tensor &features,
                        const torch::Tensor &target,
                        const BatchSchedule &schedule,
                        const char *progress_label) {
  require(!schedule.empty(), "cannot train with an empty schedule");
  head->train();
  const auto before = snapshot_parameters(head);
  torch::optim::Adam optimizer(head->parameters(),
                               torch::optim::AdamOptions(kLearningRate));
  TrainSummary summary{};
  for (std::size_t step = 0; step < schedule.size(); ++step) {
    const auto selected = index_tensor(schedule[step], features.device());
    const auto batch_features = features.index_select(0, selected);
    const auto batch_target = target.index_select(0, selected);
    optimizer.zero_grad();
    const auto prediction = forward_arm(head, batch_features, mode);
    const auto objective = compute_objective(prediction, batch_target);
    require(torch::isfinite(objective.total).item<bool>(),
            "training objective is non-finite");
    objective.total.backward();
    torch::nn::utils::clip_grad_norm_(head->parameters(), kGradClipNorm);
    optimizer.step();
    const double loss = objective.total.detach().to(torch::kCPU).item<double>();
    if (step == 0) {
      summary.first_objective = loss;
    }
    summary.final_objective = loss;
    ++summary.optimizer_steps;
    if ((step + 1) % 500 == 0) {
      std::cerr << progress_label << " step=" << (step + 1)
                << " objective=" << std::setprecision(10) << loss << '\n';
    }
  }
  summary.input_norm_parameter_max_abs_delta =
      parameter_prefix_max_abs_delta(head, before, "input_norm.");
  summary.edge_embedding_parameter_max_abs_delta =
      parameter_prefix_max_abs_delta(head, before, "edge_embedding.");
  require(summary.optimizer_steps == static_cast<int64_t>(schedule.size()),
          "optimizer step count mismatch");
  require(summary.edge_embedding_parameter_max_abs_delta == 0.0,
          "cached-feature training unexpectedly mutated edge_embedding");
  if (mode == NormalizationMode::kNone) {
    require(summary.input_norm_parameter_max_abs_delta == 0.0,
            "bypassed input_norm parameters unexpectedly changed");
  }
  return summary;
}

struct ArmResult {
  Evaluation canary_initial{};
  Evaluation canary_final{};
  Evaluation fit_initial{};
  Evaluation validation_initial{};
  Evaluation fit_final{};
  Evaluation validation_final{};
  TrainSummary canary_train{};
  TrainSummary main_train{};
  std::string initial_parameter_sha256;
  double production_manual_parity_initial_max_abs_error{0.0};
  double production_manual_parity_final_max_abs_error{0.0};
  bool canary_memorization_pass{false};
};

struct PairResult {
  ArmResult sample_layer_norm{};
  ArmResult none{};
  std::string initial_parameter_sha256;
  bool initial_parameters_byte_identical{false};
};

ArmResult run_arm(NormalizationMode mode,
                  const std::vector<int64_t> &fit_indices,
                  const std::vector<int64_t> &validation_indices,
                  const std::vector<int64_t> &canary_indices,
                  const BatchSchedule &main_schedule,
                  const BatchSchedule &canary_schedule,
                  const torch::Tensor &features, const torch::Tensor &target,
                  const torch::Device &device, const char *label,
                  const std::string &expected_initial_parameter_sha256) {
  ArmResult result{};
  {
    auto canary_head = make_head(device);
    require(canonical_parameter_digest(canary_head) ==
                expected_initial_parameter_sha256,
            "canary head initial parameter bytes differ from A/B contract");
    result.canary_initial =
        evaluate(canary_head, mode, features, target, canary_indices);
    result.canary_train =
        train_head(canary_head, mode, features, target, canary_schedule, label);
    result.canary_final =
        evaluate(canary_head, mode, features, target, canary_indices);
    result.canary_memorization_pass =
        result.canary_final.finite &&
        result.canary_final.objective_total <
            result.canary_initial.objective_total &&
        result.canary_final.metric.margin_directional_accuracy() >= 0.95 &&
        result.canary_final.metric.margin_rank_accuracy() >= 0.95;
  }

  auto head = make_head(device);
  result.initial_parameter_sha256 = canonical_parameter_digest(head);
  require(result.initial_parameter_sha256 == expected_initial_parameter_sha256,
          "trained head initial parameter bytes differ from A/B contract");
  const auto parity_indices = std::vector<int64_t>{0, 1};
  const auto parity_features =
      features.index_select(0, index_tensor(parity_indices, features.device()));
  result.production_manual_parity_initial_max_abs_error =
      production_manual_parity_max_abs_error(head, parity_features);
  require(result.production_manual_parity_initial_max_abs_error <= 1.0e-7,
          "manual normalized arithmetic differs from production head");
  result.fit_initial = evaluate(head, mode, features, target, fit_indices);
  result.validation_initial =
      evaluate(head, mode, features, target, validation_indices);
  result.main_train =
      train_head(head, mode, features, target, main_schedule, label);
  result.fit_final = evaluate(head, mode, features, target, fit_indices);
  result.validation_final =
      evaluate(head, mode, features, target, validation_indices);
  result.production_manual_parity_final_max_abs_error =
      production_manual_parity_max_abs_error(head, parity_features);
  require(result.production_manual_parity_final_max_abs_error <= 1.0e-7,
          "trained normalized arithmetic differs from production head");
  return result;
}

PairResult run_pair(const Dataset &dataset, const torch::Device &device,
                    const std::vector<int64_t> &fit_indices,
                    const std::vector<int64_t> &validation_indices,
                    const std::vector<int64_t> &canary_indices,
                    const BatchSchedule &main_schedule,
                    const BatchSchedule &canary_schedule) {
  auto sample_initial = make_head(device);
  auto none_initial = make_head(device);
  require_identical_parameters(sample_initial, none_initial);
  PairResult pair{};
  pair.initial_parameter_sha256 = canonical_parameter_digest(sample_initial);
  pair.initial_parameters_byte_identical = true;

  const auto features = dataset.features.to(device);
  const auto target = dataset.target.to(device);
  std::cerr << "running current sample-LayerNorm cached-head arm on "
            << (device.is_cuda() ? "cuda" : "cpu") << '\n';
  pair.sample_layer_norm = run_arm(
      NormalizationMode::kSampleLayerNorm, fit_indices, validation_indices,
      canary_indices, main_schedule, canary_schedule, features, target, device,
      "sample_layer_norm", pair.initial_parameter_sha256);
  std::cerr << "running exact cached-head LayerNorm-bypass arm on "
            << (device.is_cuda() ? "cuda" : "cpu") << '\n';
  pair.none = run_arm(NormalizationMode::kNone, fit_indices, validation_indices,
                      canary_indices, main_schedule, canary_schedule, features,
                      target, device, "none", pair.initial_parameter_sha256);
  return pair;
}

const char *bool_string(bool value) { return value ? "true" : "false"; }

void emit_metric(std::ostream &out, const std::string &prefix,
                 const Metric &metric) {
  out << prefix << ".count=" << metric.count << '\n';
  out << prefix << ".mae=" << metric.mae() << '\n';
  out << prefix << ".rmse=" << metric.rmse() << '\n';
  out << prefix << ".signed_error="
      << (metric.count > 0 ? metric.error_sum / metric.count : 0.0) << '\n';
  out << prefix << ".directional_accuracy=" << metric.directional_accuracy()
      << '\n';
  out << prefix << ".margin_count=" << metric.margin_count << '\n';
  out << prefix
      << ".margin_directional_accuracy=" << metric.margin_directional_accuracy()
      << '\n';
  out << prefix << ".near_zero_target_count=" << metric.near_zero_target_count
      << '\n';
  out << prefix << ".pairwise_rank_count=" << metric.pair_count << '\n';
  out << prefix << ".pairwise_rank_accuracy=" << metric.rank_accuracy() << '\n';
  out << prefix << ".margin_pairwise_rank_count=" << metric.margin_pair_count
      << '\n';
  out << prefix
      << ".margin_pairwise_rank_accuracy=" << metric.margin_rank_accuracy()
      << '\n';
  out << prefix << ".best_asset_agreement=" << metric.best_agreement() << '\n';
  out << prefix << ".correlation=" << metric.correlation() << '\n';
  out << prefix << ".prediction_mean="
      << (metric.count > 0 ? metric.prediction_sum / metric.count : 0.0)
      << '\n';
  out << prefix << ".prediction_population_std=" << metric.prediction_std()
      << '\n';
  out << prefix << ".prediction_min=" << metric.prediction_min << '\n';
  out << prefix << ".prediction_max=" << metric.prediction_max << '\n';
  out << prefix << ".target_mean="
      << (metric.count > 0 ? metric.target_sum / metric.count : 0.0) << '\n';
  out << prefix << ".target_population_std=" << metric.target_std() << '\n';
  out << prefix << ".target_min=" << metric.target_min << '\n';
  out << prefix << ".target_max=" << metric.target_max << '\n';
  out << prefix << ".prediction_to_target_std_ratio="
      << (metric.target_std() > 0.0
              ? metric.prediction_std() / metric.target_std()
              : 0.0)
      << '\n';
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    out << prefix << ".directional_accuracy.edge_" << edge << '='
        << metric.ratio(
               metric.edge_direction_correct[static_cast<std::size_t>(edge)],
               metric.edge_count[static_cast<std::size_t>(edge)])
        << '\n';
  }
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    out << prefix << ".directional_accuracy.channel_" << channel << '='
        << metric.ratio(
               metric.channel_direction_correct[static_cast<std::size_t>(
                   channel)],
               metric.channel_count[static_cast<std::size_t>(channel)])
        << '\n';
  }
}

void emit_evaluation(std::ostream &out, const std::string &prefix,
                     const Evaluation &evaluation) {
  emit_metric(out, prefix, evaluation.metric);
  out << prefix << ".objective.total=" << evaluation.objective_total << '\n';
  out << prefix << ".objective.regression=" << evaluation.objective_regression
      << '\n';
  out << prefix << ".objective.direction=" << evaluation.objective_direction
      << '\n';
  out << prefix << ".objective.rank=" << evaluation.objective_rank << '\n';
  out << prefix << ".finite=" << bool_string(evaluation.finite) << '\n';
}

void emit_train(std::ostream &out, const std::string &prefix,
                const TrainSummary &summary) {
  out << prefix << ".first_objective=" << summary.first_objective << '\n';
  out << prefix << ".final_objective=" << summary.final_objective << '\n';
  out << prefix << ".optimizer_steps=" << summary.optimizer_steps << '\n';
  out << prefix << ".input_norm_parameter_max_abs_delta="
      << summary.input_norm_parameter_max_abs_delta << '\n';
  out << prefix << ".edge_embedding_parameter_max_abs_delta="
      << summary.edge_embedding_parameter_max_abs_delta << '\n';
}

void emit_arm(std::ostream &out, const std::string &prefix,
              const ArmResult &arm) {
  out << prefix << ".initial_parameter_sha256=" << arm.initial_parameter_sha256
      << '\n';
  emit_evaluation(out, prefix + ".canary.initial", arm.canary_initial);
  emit_train(out, prefix + ".canary.train", arm.canary_train);
  emit_evaluation(out, prefix + ".canary.final", arm.canary_final);
  out << prefix << ".canary.memorization_pass="
      << bool_string(arm.canary_memorization_pass) << '\n';
  emit_evaluation(out, prefix + ".initial.fit", arm.fit_initial);
  emit_evaluation(out, prefix + ".initial.validation", arm.validation_initial);
  emit_train(out, prefix + ".train", arm.main_train);
  emit_evaluation(out, prefix + ".final.fit", arm.fit_final);
  emit_evaluation(out, prefix + ".final.validation", arm.validation_final);
  out << prefix << ".production_manual_parity_initial_max_abs_error="
      << arm.production_manual_parity_initial_max_abs_error << '\n';
  out << prefix << ".production_manual_parity_final_max_abs_error="
      << arm.production_manual_parity_final_max_abs_error << '\n';
}

void write_report(const Options &options, const Dataset &dataset,
                  const torch::Device &device,
                  const BatchSchedule &main_schedule,
                  const BatchSchedule &canary_schedule,
                  const PairResult &pair) {
  std::filesystem::create_directories(options.output.parent_path());
  std::ofstream out(options.output, std::ios::trunc);
  require(out.good(), "cannot open output report");
  out << std::setprecision(12) << std::fixed;
  out << "schema_id=" << options.schema_id << '\n';
  out << "status=complete\n";
  out << "benchmark_id=synthetic_continuous_graph_v1\n";
  out << "diagnostic_authority=development_only\n";
  out << "benchmark_acceptance_authority=false\n";
  out << "question=does_the_per_sample_LayerNorm_in_DirectEdgeReturnHead_"
         "destroy_decodable_signal_at_the_exact_cached_post_adapter_boundary\n";
  out << '\n';
  out << "scope.boundary=exact_cached_post_adapter_direct_edge_readout_input\n";
  out << "scope.causal_variable=input_norm_forward_call_only\n";
  out << "scope.fixed_cached_features=true\n";
  out << "scope.identical_initial_parameter_bytes=true\n";
  out << "scope.identical_precomputed_batch_schedule=true\n";
  out << "scope.single_seed=true\n";
  out << "scope.end_to_end_representation_cause_established=false\n";
  out << "scope.production_retraining_effect_established=false\n";
  out << '\n';
  out << "provenance.policy_path_used=false\n";
  out << "provenance.checkpoint_loaded_by_helper=false\n";
  out << "provenance.checkpoint_written=false\n";
  out << "provenance.model_artifact_written=false\n";
  out << "provenance.upstream_frozen_capture_used=true\n";
  out << "provenance.upstream_representation_checkpoint_used_by_capture=true\n";
  out << "provenance.upstream_mdn_adapter_and_edge_embedding_checkpoint_used_"
         "by_capture=true\n";
  out << "provenance.upstream_representation_fit_anchor_range=[0,730)\n";
  out << "provenance.upstream_mdn_fit_anchor_range=[0,730)\n";
  out << "provenance.validation_upstream_exposure=transductive\n";
  out << "provenance.validation_held_out_from_new_head_optimizer_only=true\n";
  out << "provenance.historical_input_used=false\n";
  out << "provenance.holdout_input_used=false\n";
  out << "provenance.capture_model_state_mutated=false\n";
  out << "provenance.benchmark_contracts_validated_and_sealed_by_runner=true\n";
  out << '\n';
  out << "data.record_schema=kikijyeba.synthetic.mdn_edge_context_feature_"
         "probe.v1\n";
  out << "data.anchor_range=[0,730)\n";
  out << "data.row_count=" << kExpectedRows << '\n';
  out << "data.edge_count=" << kEdgeCount << '\n';
  out << "data.channel_count=" << kChannelCount << '\n';
  out << "data.hidden_width=" << kHiddenWidth << '\n';
  out << "data.identity_embedding_width=" << kIdentityEmbeddingWidth << '\n';
  out << "data.readout_feature_count=" << kReadoutFeatureCount << '\n';
  out << "data.feature_layout=base[0:128],quote[128:256],diff[256:384],"
         "cached_edge_embedding[384:400]\n";
  out << "data.cached_identity_embedding_source=upstream_trained_mdn_"
         "checkpoint\n";
  out << "data.new_head_edge_embedding_parameters_used_in_forward=false\n";
  out << "data.new_head_edge_embedding_parameters_retained=true\n";
  out << "data.target_definition=captured_next_close_edge_return\n";
  out << "data.input_sha256=" << dataset.input_sha256 << '\n';
  out << "data.production_head_header_sha256="
      << sha256_file(options.head_header) << '\n';
  out << "data.dynamic_difference_max_abs_error="
      << dataset.dynamic_difference_max_abs_error << '\n';
  out << "data.quote_consistency_max_abs_error="
      << dataset.quote_consistency_max_abs_error << '\n';
  out << "data.identity_consistency_max_abs_error="
      << dataset.identity_consistency_max_abs_error << '\n';
  out << "data.identity_min_pair_distance="
      << dataset.identity_min_pair_distance << '\n';
  out << "data.self_test_passed=true\n";
  out << '\n';
  out << "boundary.fit=[" << kFitBegin << ',' << kFitEnd << ")\n";
  out << "boundary.purge=[" << kFitEnd << ',' << kPurgeEnd << ")\n";
  out << "boundary.validation=[" << kPurgeEnd << ',' << kValidationEnd << ")\n";
  out << "boundary.historical=[760,1088)\n";
  out << "boundary.historical_opened=false\n";
  out << "boundary.unconsumed_holdout=[1088,1170)\n";
  out << "boundary.unconsumed_holdout_opened=false\n";
  out << '\n';
  out << "execution.device=" << (device.is_cuda() ? "cuda" : "cpu") << '\n';
  out << "execution.dtype=float32\n";
  out << "execution.seed=" << kSeed << '\n';
  out << "execution.identity_mode=edge_embedding_per_edge\n";
  out << "execution.adapter_reapplied=false\n";
  out << "execution.input_norm_module_registered_both_arms=true\n";
  out << "execution.input_norm_parameters_retained_both_arms=true\n";
  out << "execution.sample_layer_norm_arm_calls_production_forward=true\n";
  out << "execution.none_arm_bypasses_only_input_norm_forward=true\n";
  out << "execution.hidden_width=" << kHiddenWidth << '\n';
  out << "execution.steps=" << kTrainingSteps << '\n';
  out << "execution.batch_size=" << kBatchSize << '\n';
  out << "execution.learning_rate=" << kLearningRate << '\n';
  out << "execution.grad_clip_norm=" << kGradClipNorm << '\n';
  out << "execution.optimizer=Adam\n";
  out << "execution.objective_regression_weight=" << kRegressionWeight << '\n';
  out << "execution.objective_direction_weight=" << kDirectionWeight << '\n';
  out << "execution.objective_rank_weight=" << kRankWeight << '\n';
  out << "execution.objective_huber_beta=" << kHuberBeta << '\n';
  out << "execution.objective_logit_scale=" << kLogitScale << '\n';
  out << "execution.objective_target_scale=" << kTargetScale << '\n';
  out << "execution.schedule_sha256=" << schedule_digest(main_schedule) << '\n';
  out << "execution.schedule_batch_count=" << main_schedule.size() << '\n';
  out << "execution.canary_anchor_range=[" << kCanaryBegin << ',' << kCanaryEnd
      << ")\n";
  out << "execution.canary_steps=" << kCanarySteps << '\n';
  out << "execution.canary_schedule_sha256=" << schedule_digest(canary_schedule)
      << '\n';
  out << "execution.initial_parameter_sha256=" << pair.initial_parameter_sha256
      << '\n';
  out << "execution.initial_parameters_byte_identical="
      << bool_string(pair.initial_parameters_byte_identical) << '\n';
  out << '\n';
  emit_arm(out, "arm.sample_layer_norm", pair.sample_layer_norm);
  out << '\n';
  emit_arm(out, "arm.none", pair.none);
  out << '\n';
  const auto &normalized = pair.sample_layer_norm.validation_final.metric;
  const auto &none = pair.none.validation_final.metric;
  const double direction_delta = none.margin_directional_accuracy() -
                                 normalized.margin_directional_accuracy();
  const double rank_delta =
      none.margin_rank_accuracy() - normalized.margin_rank_accuracy();
  const double correlation_delta =
      none.correlation() - normalized.correlation();
  const double rmse_delta = none.rmse() - normalized.rmse();
  const bool actual_initial_parameters_match =
      pair.sample_layer_norm.initial_parameter_sha256 ==
          pair.initial_parameter_sha256 &&
      pair.none.initial_parameter_sha256 == pair.initial_parameter_sha256;
  const bool main_evaluations_finite =
      pair.sample_layer_norm.fit_final.finite &&
      pair.sample_layer_norm.validation_final.finite &&
      pair.none.fit_final.finite && pair.none.validation_final.finite;
  const bool classification_evidence_valid =
      pair.initial_parameters_byte_identical &&
      actual_initial_parameters_match && main_evaluations_finite &&
      pair.none.canary_memorization_pass;
  const bool major_conditioning_loss =
      classification_evidence_valid && direction_delta >= 0.05 &&
      rank_delta >= 0.05 && correlation_delta >= 0.10;
  out << "ablation.delta.none_minus_sample_layer_norm.validation.margin_"
         "directional_accuracy="
      << direction_delta << '\n';
  out << "ablation.delta.none_minus_sample_layer_norm.validation.margin_"
         "pairwise_rank_accuracy="
      << rank_delta << '\n';
  out << "ablation.delta.none_minus_sample_layer_norm.validation.correlation="
      << correlation_delta << '\n';
  out << "ablation.delta.none_minus_sample_layer_norm.validation.rmse="
      << rmse_delta << '\n';
  out << "classification.actual_arm_initial_parameters_match="
      << bool_string(actual_initial_parameters_match) << '\n';
  out << "classification.main_evaluations_finite="
      << bool_string(main_evaluations_finite) << '\n';
  out << "classification.bypass_capacity_canary_pass="
      << bool_string(pair.none.canary_memorization_pass) << '\n';
  out << "classification.evidence_valid="
      << bool_string(classification_evidence_valid) << '\n';
  out << "classification.cached_boundary_sample_layer_norm_major_"
         "conditioning_loss_observed="
      << bool_string(major_conditioning_loss) << '\n';
  out << "classification.causal_scope=fixed_cached_features_identical_"
         "initial_parameters_and_schedule_single_forward_call_ablation\n";
  out << "classification.transductive_caveat=validation_was_seen_by_upstream_"
         "representation_and_mdn_adapter_training\n";
  out << "classification.narrow_conclusion="
      << (classification_evidence_valid
              ? "valid_evidence_estimates_only_the_effect_of_sample_"
                "LayerNorm_at_the_cached_direct_head_boundary"
              : "no_cached_boundary_effect_conclusion_because_diagnostic_"
                "validity_gate_failed")
      << '\n';
  out << "classification.end_to_end_conclusion=not_established\n";
  out << "summary=development_only_cached_boundary_input_norm_ablation_"
         "without_policy_checkpoint_write_history_or_holdout\n";
  require(out.good(), "failed while writing output report");
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    at::globalContext().setBenchmarkCuDNN(false);
    at::globalContext().setDeterministicCuDNN(true);
    at::globalContext().setDeterministicAlgorithms(true, false);
    at::globalContext().setDeterministicFillUninitializedMemory(true);
    const torch::Device device =
        !options.force_cpu && torch::cuda::is_available()
            ? torch::Device(torch::kCUDA)
            : torch::Device(torch::kCPU);
    const auto dataset = read_dataset(options);
    const auto fit_indices = range_indices(kFitBegin, kFitEnd);
    const auto validation_indices = range_indices(kPurgeEnd, kValidationEnd);
    const auto canary_indices = range_indices(kCanaryBegin, kCanaryEnd);
    const auto main_schedule =
        make_schedule(fit_indices, kTrainingSteps, kSeed);
    const auto canary_schedule =
        make_schedule(canary_indices, kCanarySteps, kSeed);
    const auto result =
        run_pair(dataset, device, fit_indices, validation_indices,
                 canary_indices, main_schedule, canary_schedule);
    write_report(options, dataset, device, main_schedule, canary_schedule,
                 result);
    std::cout << "status=complete\n";
    std::cout << "schema_id=" << options.schema_id << '\n';
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "torch error: " << error.what() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << '\n';
  }
  return 1;
}
