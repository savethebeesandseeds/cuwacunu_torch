// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
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
#include "wikimyei/inference/expected_value/mdn/channel_context_mdn.h"
#include "wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h"

namespace {

namespace digest = cuwacunu::piaabo::digest;
namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;

constexpr int64_t kRequiredPrefixRows = 3294;
constexpr int64_t kInputLength = 29;
constexpr int64_t kDeclaredTrainBegin = 0;
constexpr int64_t kEffectiveTrainBegin = 1;
constexpr int64_t kTrainEnd = 2496;
constexpr int64_t kValidationBegin = 2560;
constexpr int64_t kValidationEnd = 2816;
constexpr int64_t kCertifiedBegin = 2880;
constexpr int64_t kCertifiedEnd = 3264;
constexpr int64_t kMaximumAnchorLoaded = kCertifiedEnd - 1;
constexpr int64_t kMaximumSourceRowLoaded =
    kMaximumAnchorLoaded + kInputLength + 1;
constexpr int64_t kFinalHoldoutBegin = 3328;
constexpr int64_t kNodeCount = 4;
constexpr int64_t kChannelCount = 1;
constexpr int64_t kTargetFeatureCount = 1;
constexpr int64_t kRawCloseCoord = 3;
constexpr int64_t kKlineFeatureWidth = 9;
constexpr int64_t kEdgeCount = 3;
constexpr int64_t kFlatFeatureCount = kInputLength * kEdgeCount;
constexpr int64_t kHiddenWidth = 128;
constexpr int64_t kResidualDepth = 2;
constexpr int64_t kFeatureEmbeddingDim = 8;
constexpr int64_t kChannelAdapterRank = 16;
constexpr int64_t kTrainingSteps = 3500;
constexpr int64_t kBatchSize = 64;
constexpr int64_t kCanaryBegin = 1;
constexpr int64_t kCanaryEnd = 33;
constexpr int64_t kCanarySteps = 750;
constexpr int64_t kSeed = 31;
constexpr double kLearningRate = 1.0e-3;
constexpr double kGradClipNorm = 5.0;
constexpr double kSigmaFloor = 1.0e-3;
constexpr double kLinearRidge = 1.0e-6;
constexpr double kDirectRegressionWeight = 100.0;
constexpr double kDirectDirectionWeight = 5.0;
constexpr double kDirectRankWeight = 5.0;
constexpr double kDirectHuberBeta = 0.5;
constexpr double kDirectLogitScale = 1.0;
constexpr double kDirectTargetScale = 36.0;
constexpr int64_t kDirectWarmupSteps = 800;
constexpr int64_t kDirectIdentityEmbeddingDim = 16;
constexpr int64_t kDirectAdapterHiddenDim = 128;
constexpr double kDirectionGate = 0.95;
constexpr double kRankGate = 0.95;
constexpr double kCorrelationGate = 0.95;
constexpr double kRmseRatioGate = 0.25;

struct Options {
  std::filesystem::path alpha_prefix;
  std::filesystem::path beta_prefix;
  std::filesystem::path gamma_prefix;
  std::filesystem::path closure;
  std::filesystem::path output;
  std::string schema_id{"synthetic_v2_raw_history_supervised_isolation_v1"};
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

const char *bool_string(bool value) { return value ? "true" : "false"; }

std::string required_value(int argc, char **argv, int &index,
                           const std::string &flag) {
  if (index + 1 >= argc) {
    fail("missing value for " + flag);
  }
  return argv[++index];
}

Options parse_options(int argc, char **argv) {
  Options options{};
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--alpha-prefix") {
      options.alpha_prefix = required_value(argc, argv, index, argument);
    } else if (argument == "--beta-prefix") {
      options.beta_prefix = required_value(argc, argv, index, argument);
    } else if (argument == "--gamma-prefix") {
      options.gamma_prefix = required_value(argc, argv, index, argument);
    } else if (argument == "--closure") {
      options.closure = required_value(argc, argv, index, argument);
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
  require(!options.alpha_prefix.empty(), "--alpha-prefix is required");
  require(!options.beta_prefix.empty(), "--beta-prefix is required");
  require(!options.gamma_prefix.empty(), "--gamma-prefix is required");
  require(!options.closure.empty(), "--closure is required");
  require(!options.output.empty(), "--output is required");
  require(options.schema_id ==
              "synthetic_v2_raw_history_supervised_isolation_v1",
          "unexpected schema id");
  return options;
}

bool ends_with(const std::string &value, const std::string &suffix) {
  return value.size() >= suffix.size() &&
         value.compare(value.size() - suffix.size(), suffix.size(), suffix) ==
             0;
}

void reject_symlink_components(const std::filesystem::path &path,
                               const std::string &label) {
  require(path.is_absolute(), label + " must be absolute");
  std::filesystem::path current = path.root_path();
  for (const auto &component : path.relative_path()) {
    current /= component;
    const auto status = std::filesystem::symlink_status(current);
    require(status.type() != std::filesystem::file_type::symlink,
            label + " traverses a symlink component: " + current.string());
  }
}

void validate_exact_input_path(const std::filesystem::path &path,
                               const std::string &suffix,
                               const std::string &label) {
  require(path.is_absolute(), label + " must be absolute");
  require(path.lexically_normal() == path,
          label + " must be lexically normalized");
  reject_symlink_components(path, label);
  require(std::filesystem::is_regular_file(path),
          label + " is not a regular file: " + path.string());
  const auto canonical = std::filesystem::canonical(path);
  require(canonical == path,
          label + " is not its exact canonical path: " + path.string());
  const std::string text = canonical.string();
  require(text.find("/data/raw/") == std::string::npos,
          label + " points into forbidden data/raw");
  require(ends_with(text, suffix),
          label + " does not have the required exact development path");
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

void validate_inputs(const Options &options) {
  validate_exact_input_path(
      options.alpha_prefix,
      "/data/development_prefix/SYN2ALPHASYN2USD/1d/"
      "SYN2ALPHASYN2USD-1d-all-years.csv",
      "alpha development prefix");
  validate_exact_input_path(
      options.beta_prefix,
      "/data/development_prefix/SYN2BETASYN2USD/1d/"
      "SYN2BETASYN2USD-1d-all-years.csv",
      "beta development prefix");
  validate_exact_input_path(
      options.gamma_prefix,
      "/data/development_prefix/SYN2GAMMASYN2USD/1d/"
      "SYN2GAMMASYN2USD-1d-all-years.csv",
      "gamma development prefix");
  validate_exact_input_path(
      options.closure,
      "/artifacts/fresh_seed_data_closure.v2.report",
      "fresh-seed data closure");
  require(!std::filesystem::equivalent(options.alpha_prefix,
                                       options.beta_prefix) &&
              !std::filesystem::equivalent(options.alpha_prefix,
                                           options.gamma_prefix) &&
              !std::filesystem::equivalent(options.beta_prefix,
                                           options.gamma_prefix),
          "development-prefix inputs must be distinct files");
  const std::string closure = read_file(options.closure);
  require(closure.find("schema_id=synthetic_fresh_seed_data_closure.v2\n") !=
              std::string::npos,
          "unexpected fresh-seed closure schema");
  require(closure.find("accepted_anchor_count=4096\n") != std::string::npos,
          "fresh-seed closure has an unexpected anchor count");
  require(closure.find("closure_complete=true\n") != std::string::npos,
          "fresh-seed closure is not complete");
}

struct KlineRow {
  int64_t open_time{0};
  double close{0.0};
  int64_t close_time{0};
};

int64_t parse_int64_field(const std::string &token, int64_t row_index,
                          const char *field_name) {
  std::size_t consumed = 0;
  const int64_t value = std::stoll(token, &consumed);
  require(consumed == token.size(), std::string("invalid ") + field_name +
                                        " at prefix row " +
                                        std::to_string(row_index));
  return value;
}

KlineRow parse_kline_row(const std::string &line, int64_t row_index) {
  std::istringstream row(line);
  std::array<std::string, 12> fields{};
  for (std::size_t field = 0; field < fields.size(); ++field) {
    require(static_cast<bool>(std::getline(row, fields[field], ',')),
            "malformed kline row " + std::to_string(row_index));
  }
  std::string trailing;
  require(!std::getline(row, trailing, ','),
          "kline row has more than 12 fields at prefix row " +
              std::to_string(row_index));

  KlineRow parsed{};
  parsed.open_time = parse_int64_field(fields[0], row_index, "open_time");
  std::size_t close_consumed = 0;
  parsed.close = std::stod(fields[4], &close_consumed);
  require(close_consumed == fields[4].size() && std::isfinite(parsed.close) &&
              parsed.close > 0.0,
          "invalid close at prefix row " + std::to_string(row_index));
  parsed.close_time = parse_int64_field(fields[6], row_index, "close_time");
  return parsed;
}

std::vector<KlineRow> read_kline_prefix(const std::filesystem::path &path) {
  std::ifstream input(path);
  require(input.good(), "cannot open prefix: " + path.string());
  std::vector<KlineRow> rows;
  rows.reserve(kRequiredPrefixRows);
  std::string line;
  for (int64_t row = 0; row < kRequiredPrefixRows; ++row) {
    require(static_cast<bool>(std::getline(input, line)),
            "development prefix ended before row " + std::to_string(row));
    rows.push_back(parse_kline_row(line, row));
  }
  require(!std::getline(input, line),
          "development prefix contains more than 3294 rows");
  return rows;
}

std::array<double, kNodeCount>
uniform_gauge_lift(const std::array<double, kEdgeCount> &edge_returns) {
  const double quote =
      -(edge_returns[0] + edge_returns[1] + edge_returns[2]) / 4.0;
  return {quote, edge_returns[0] + quote, edge_returns[1] + quote,
          edge_returns[2] + quote};
}

struct Dataset {
  torch::Tensor context;              // [3264,4,1,29]
  torch::Tensor context_mask;         // [3264,4,1]
  torch::Tensor future_node_features; // [3264,1,1,4,9]
  torch::Tensor future_node_mask;     // [3264,1,1,4,9]
  torch::Tensor anchor_keys;          // [3264]
  std::vector<std::array<double, kFlatFeatureCount>> edge_features;
  std::vector<std::array<double, kEdgeCount>> edge_targets;
  std::array<std::string, kEdgeCount> prefix_sha256{};
  std::string closure_sha256;
  double projection_max_abs_error{0.0};
  double float_projection_max_abs_error{0.0};
};

Dataset build_dataset(const Options &options) {
  constexpr int64_t kOneDayMs = 86400000;
  const std::array<std::vector<KlineRow>, kEdgeCount> rows{
      read_kline_prefix(options.alpha_prefix),
      read_kline_prefix(options.beta_prefix),
      read_kline_prefix(options.gamma_prefix)};
  for (int64_t row = 0; row < kRequiredPrefixRows; ++row) {
    const auto &reference = rows[0][static_cast<std::size_t>(row)];
    require(reference.close_time == reference.open_time + kOneDayMs - 1,
            "1d close-time cadence mismatch at prefix row " +
                std::to_string(row));
    if (row > 0) {
      const auto &previous = rows[0][static_cast<std::size_t>(row - 1)];
      require(reference.open_time - previous.open_time == kOneDayMs,
              "1d open-time cadence mismatch at prefix row " +
                  std::to_string(row));
    }
    for (std::size_t instrument = 1; instrument < rows.size(); ++instrument) {
      const auto &candidate = rows[instrument][static_cast<std::size_t>(row)];
      require(candidate.open_time == reference.open_time &&
                  candidate.close_time == reference.close_time,
              "cross-instrument timestamp alignment failed at prefix row " +
                  std::to_string(row));
    }
  }

  Dataset dataset{};
  dataset.prefix_sha256 = {sha256_file(options.alpha_prefix),
                           sha256_file(options.beta_prefix),
                           sha256_file(options.gamma_prefix)};
  dataset.closure_sha256 = sha256_file(options.closure);
  dataset.context = torch::zeros(
      {kCertifiedEnd, kNodeCount, kChannelCount, kInputLength},
      torch::kFloat32);
  dataset.context_mask = torch::ones(
      {kCertifiedEnd, kNodeCount, kChannelCount}, torch::kBool);
  dataset.future_node_features = torch::zeros(
      {kCertifiedEnd, kChannelCount, 1, kNodeCount, kKlineFeatureWidth},
      torch::kFloat32);
  dataset.future_node_mask = torch::zeros(
      {kCertifiedEnd, kChannelCount, 1, kNodeCount, kKlineFeatureWidth},
      torch::kBool);
  dataset.anchor_keys = torch::empty(
      {kCertifiedEnd}, torch::TensorOptions().dtype(torch::kInt64));
  dataset.edge_features.resize(kCertifiedEnd);
  dataset.edge_targets.resize(kCertifiedEnd);

  auto context = dataset.context.accessor<float, 4>();
  auto future = dataset.future_node_features.accessor<float, 5>();
  auto future_mask = dataset.future_node_mask.accessor<bool, 5>();
  auto anchor_keys = dataset.anchor_keys.accessor<int64_t, 1>();

  for (int64_t anchor = 0; anchor < kCertifiedEnd; ++anchor) {
    anchor_keys[anchor] =
        rows[0][static_cast<std::size_t>(anchor + kInputLength)].close_time;
    for (int64_t history = 0; history < kInputLength; ++history) {
      const int64_t current_row = anchor + history + 1;
      std::array<double, kEdgeCount> edges{};
      for (std::size_t edge = 0; edge < edges.size(); ++edge) {
        edges[edge] = std::log(
            rows[edge][static_cast<std::size_t>(current_row)].close /
            rows[edge][static_cast<std::size_t>(current_row - 1)].close);
        dataset.edge_features[static_cast<std::size_t>(anchor)]
                             [edge * kInputLength + history] = edges[edge];
      }
      const auto nodes = uniform_gauge_lift(edges);
      for (int64_t node = 0; node < kNodeCount; ++node) {
        context[anchor][node][0][history] =
            static_cast<float>(nodes[static_cast<std::size_t>(node)]);
      }
      for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
        const double reconstructed =
            nodes[static_cast<std::size_t>(edge + 1)] - nodes[0];
        dataset.projection_max_abs_error =
            std::max(dataset.projection_max_abs_error,
                     std::abs(reconstructed -
                              edges[static_cast<std::size_t>(edge)]));
        const double reconstructed_float =
            static_cast<double>(context[anchor][edge + 1][0][history]) -
            static_cast<double>(context[anchor][0][0][history]);
        dataset.float_projection_max_abs_error =
            std::max(dataset.float_projection_max_abs_error,
                     std::abs(reconstructed_float -
                              edges[static_cast<std::size_t>(edge)]));
      }
    }

    const int64_t target_row = anchor + kInputLength + 1;
    std::array<double, kEdgeCount> target_edges{};
    for (std::size_t edge = 0; edge < target_edges.size(); ++edge) {
      target_edges[edge] = std::log(
          rows[edge][static_cast<std::size_t>(target_row)].close /
          rows[edge][static_cast<std::size_t>(target_row - 1)].close);
    }
    dataset.edge_targets[static_cast<std::size_t>(anchor)] = target_edges;
    const auto target_nodes = uniform_gauge_lift(target_edges);
    for (int64_t node = 0; node < kNodeCount; ++node) {
      future[anchor][0][0][node][kRawCloseCoord] =
          static_cast<float>(target_nodes[static_cast<std::size_t>(node)]);
      future_mask[anchor][0][0][node][kRawCloseCoord] = true;
    }
  }

  require(dataset.context.sizes() ==
              torch::IntArrayRef(
                  {kCertifiedEnd, kNodeCount, 1, kInputLength}),
          "raw context shape mismatch");
  require(dataset.future_node_features.sizes() ==
              torch::IntArrayRef({kCertifiedEnd, 1, 1, kNodeCount,
                                  kKlineFeatureWidth}),
          "future source shape mismatch");
  require(dataset.projection_max_abs_error <= 1.0e-12,
          "uniform-gauge projection identity failed");
  require(dataset.float_projection_max_abs_error <= 1.0e-7,
          "float uniform-gauge projection drift is too large");
  require(kMaximumAnchorLoaded == 3263,
          "maximum anchor boundary changed");
  require(kMaximumSourceRowLoaded == 3293,
          "maximum source-row boundary changed");
  require(kMaximumSourceRowLoaded < kRequiredPrefixRows,
          "source-row exposure exceeds development prefix");
  require(kCertifiedEnd <= kFinalHoldoutBegin,
          "certified evaluation overlaps final holdout");
  return dataset;
}

std::vector<int64_t> range_indices(int64_t begin, int64_t end) {
  require(begin >= 0 && end >= begin && end <= kCertifiedEnd,
          "invalid anchor range");
  std::vector<int64_t> indices(static_cast<std::size_t>(end - begin));
  std::iota(indices.begin(), indices.end(), begin);
  return indices;
}

torch::Tensor index_tensor(const std::vector<int64_t> &indices) {
  return torch::tensor(indices, torch::TensorOptions().dtype(torch::kInt64));
}

mdn::channel_mdn_input_t make_input(const Dataset &dataset,
                                    const std::vector<int64_t> &indices,
                                    const torch::Device &device) {
  require(!indices.empty(), "cannot make an empty MDN input");
  const auto index = index_tensor(indices);
  return mdn::make_channel_mdn_input(
      dataset.context.index_select(0, index).to(device),
      dataset.context_mask.index_select(0, index).to(device),
      dataset.future_node_features.index_select(0, index).to(device),
      dataset.future_node_mask.index_select(0, index).to(device),
      std::vector<int64_t>{kRawCloseCoord},
      dataset.anchor_keys.index_select(0, index).to(device),
      std::vector<std::string>{"SYN2USD", "SYN2ALPHA", "SYN2BETA",
                               "SYN2GAMMA"});
}

int sign_of(double value) { return value > 0.0 ? 1 : (value < 0.0 ? -1 : 0); }

struct EdgeMetric {
  int64_t count{0};
  int64_t direction_correct{0};
  int64_t pair_count{0};
  int64_t pair_correct{0};
  int64_t best_count{0};
  int64_t best_correct{0};
  double abs_error_sum{0.0};
  double squared_error_sum{0.0};
  double prediction_sum{0.0};
  double target_sum{0.0};
  double prediction_squared_sum{0.0};
  double target_squared_sum{0.0};
  double cross_sum{0.0};

  void observe(const std::array<double, kEdgeCount> &prediction,
               const std::array<double, kEdgeCount> &target) {
    for (std::size_t edge = 0; edge < prediction.size(); ++edge) {
      const double p = prediction[edge];
      const double y = target[edge];
      require(std::isfinite(p) && std::isfinite(y),
              "non-finite edge metric input");
      const double error = p - y;
      ++count;
      abs_error_sum += std::abs(error);
      squared_error_sum += error * error;
      prediction_sum += p;
      target_sum += y;
      prediction_squared_sum += p * p;
      target_squared_sum += y * y;
      cross_sum += p * y;
      if (sign_of(p) == sign_of(y)) {
        ++direction_correct;
      }
    }
    for (std::size_t lhs = 0; lhs < prediction.size(); ++lhs) {
      for (std::size_t rhs = lhs + 1; rhs < prediction.size(); ++rhs) {
        ++pair_count;
        if (sign_of(prediction[lhs] - prediction[rhs]) ==
            sign_of(target[lhs] - target[rhs])) {
          ++pair_correct;
        }
      }
    }
    const auto predicted_best = static_cast<int64_t>(std::distance(
        prediction.begin(),
        std::max_element(prediction.begin(), prediction.end())));
    const auto target_best = static_cast<int64_t>(std::distance(
        target.begin(), std::max_element(target.begin(), target.end())));
    ++best_count;
    if (predicted_best == target_best) {
      ++best_correct;
    }
  }

  double mae() const {
    return count > 0 ? abs_error_sum / static_cast<double>(count)
                     : std::numeric_limits<double>::quiet_NaN();
  }
  double rmse() const {
    return count > 0 ? std::sqrt(squared_error_sum / static_cast<double>(count))
                     : std::numeric_limits<double>::quiet_NaN();
  }
  double target_rms() const {
    return count > 0
               ? std::sqrt(target_squared_sum / static_cast<double>(count))
               : std::numeric_limits<double>::quiet_NaN();
  }
  double rmse_to_target_rms() const {
    const double scale = target_rms();
    return scale > 0.0 ? rmse() / scale
                       : std::numeric_limits<double>::quiet_NaN();
  }
  double direction() const {
    return count > 0 ? static_cast<double>(direction_correct) /
                           static_cast<double>(count)
                     : std::numeric_limits<double>::quiet_NaN();
  }
  double rank() const {
    return pair_count > 0 ? static_cast<double>(pair_correct) /
                                static_cast<double>(pair_count)
                          : std::numeric_limits<double>::quiet_NaN();
  }
  double best() const {
    return best_count > 0 ? static_cast<double>(best_correct) /
                                static_cast<double>(best_count)
                          : std::numeric_limits<double>::quiet_NaN();
  }
  double correlation() const {
    if (count <= 1) {
      return std::numeric_limits<double>::quiet_NaN();
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
  double prediction_rms() const {
    return count > 0
               ? std::sqrt(prediction_squared_sum / static_cast<double>(count))
               : std::numeric_limits<double>::quiet_NaN();
  }
  bool finite() const {
    return count > 0 && pair_count > 0 && std::isfinite(mae()) &&
           std::isfinite(rmse()) && std::isfinite(target_rms()) &&
           std::isfinite(direction()) && std::isfinite(rank()) &&
           std::isfinite(correlation()) &&
           std::isfinite(rmse_to_target_rms());
  }
};

bool capability_gate(const EdgeMetric &metric) {
  return metric.finite() && metric.direction() >= kDirectionGate &&
         metric.rank() >= kRankGate &&
         metric.correlation() >= kCorrelationGate &&
         metric.rmse_to_target_rms() <= kRmseRatioGate;
}

struct Evaluation {
  EdgeMetric expectation_edge{};
  EdgeMetric direct_edge{};
  double mean_nll{std::numeric_limits<double>::quiet_NaN()};
  double sigma_min{std::numeric_limits<double>::quiet_NaN()};
  double sigma_mean{std::numeric_limits<double>::quiet_NaN()};
  double sigma_max{std::numeric_limits<double>::quiet_NaN()};
  double mixture_entropy{std::numeric_limits<double>::quiet_NaN()};
  std::vector<double> mixture_usage{};
  bool finite{false};
};

Evaluation evaluate(mdn::ChannelContextMdn &model, const Dataset &dataset,
                    const std::vector<int64_t> &indices,
                    const torch::Device &device, int64_t mixture_count) {
  auto input = make_input(dataset, indices, device);
  model->eval();
  torch::NoGradGuard no_grad;
  const auto output = model->forward(input.context, input.context_mask);
  const auto combined_mask = mdn::combine_channel_context_and_future_mask(
      input.context_mask, input.future_mask);
  const auto nll_map = mdn::mdn_nll_map(output, input.future);
  const auto valid_nll = nll_map.masked_select(combined_mask);
  require(valid_nll.numel() > 0, "evaluation has no valid NLL targets");

  Evaluation evaluation{};
  evaluation.mean_nll =
      valid_nll.mean().to(torch::kCPU).template item<double>();
  const auto sigma_mask = combined_mask.unsqueeze(-1).expand_as(output.sigma);
  const auto valid_sigma = output.sigma.masked_select(sigma_mask);
  require(valid_sigma.numel() > 0, "evaluation has no valid sigma values");
  evaluation.sigma_min =
      valid_sigma.min().to(torch::kCPU).template item<double>();
  evaluation.sigma_mean =
      valid_sigma.mean().to(torch::kCPU).template item<double>();
  evaluation.sigma_max =
      valid_sigma.max().to(torch::kCPU).template item<double>();

  const auto pi = output.log_pi.exp();
  const auto entropy = -(pi * output.log_pi).sum(-1);
  evaluation.mixture_entropy = entropy.masked_select(combined_mask)
                                   .mean()
                                   .to(torch::kCPU)
                                   .template item<double>();
  const auto valid_weight = combined_mask.to(pi.dtype()).unsqueeze(-1);
  const auto usage = (pi * valid_weight).sum(std::vector<int64_t>{0, 1, 2, 3}) /
                     combined_mask.sum().to(pi.dtype()).clamp_min(1.0);
  const auto usage_cpu = usage.to(torch::kCPU).to(torch::kFloat64).contiguous();
  require(usage_cpu.numel() == mixture_count, "mixture usage shape mismatch");
  const auto usage_accessor = usage_cpu.accessor<double, 1>();
  for (int64_t component = 0; component < mixture_count; ++component) {
    evaluation.mixture_usage.push_back(usage_accessor[component]);
  }

  const auto expected = mdn::mdn_expectation(output)
                            .to(torch::kCPU)
                            .to(torch::kFloat64)
                            .contiguous();
  require(output.direct_edge_return.defined() &&
              output.direct_edge_return.dim() == 3 &&
              output.direct_edge_return.size(0) == expected.size(0) &&
              output.direct_edge_return.size(1) == kEdgeCount &&
              output.direct_edge_return.size(2) == kChannelCount,
          "direct edge output shape mismatch");
  const auto direct = output.direct_edge_return
                          .to(torch::kCPU)
                          .to(torch::kFloat64)
                          .contiguous();
  const auto realized =
      input.future.to(torch::kCPU).to(torch::kFloat64).contiguous();
  const auto valid = combined_mask.to(torch::kCPU).contiguous();
  const auto expected_a = expected.accessor<double, 4>();
  const auto direct_a = direct.accessor<double, 3>();
  const auto realized_a = realized.accessor<double, 4>();
  const auto valid_a = valid.accessor<bool, 4>();
  for (int64_t sample = 0; sample < expected.size(0); ++sample) {
    std::array<double, kEdgeCount> prediction{};
    std::array<double, kEdgeCount> direct_prediction{};
    std::array<double, kEdgeCount> target{};
    require(valid_a[sample][0][0][0], "quote target is invalid");
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      require(valid_a[sample][edge + 1][0][0], "base target is invalid");
      prediction[static_cast<std::size_t>(edge)] =
          expected_a[sample][edge + 1][0][0] -
          expected_a[sample][0][0][0];
      direct_prediction[static_cast<std::size_t>(edge)] =
          direct_a[sample][edge][0];
      target[static_cast<std::size_t>(edge)] =
          realized_a[sample][edge + 1][0][0] -
          realized_a[sample][0][0][0];
    }
    evaluation.expectation_edge.observe(prediction, target);
    evaluation.direct_edge.observe(direct_prediction, target);
  }
  evaluation.finite =
      std::isfinite(evaluation.mean_nll) &&
      std::isfinite(evaluation.sigma_min) &&
      std::isfinite(evaluation.sigma_mean) &&
      std::isfinite(evaluation.sigma_max) &&
      std::isfinite(evaluation.mixture_entropy) &&
      evaluation.expectation_edge.finite() && evaluation.direct_edge.finite() &&
      evaluation.expectation_edge.count ==
          static_cast<int64_t>(indices.size()) * 3 &&
      evaluation.direct_edge.count == static_cast<int64_t>(indices.size()) * 3;
  return evaluation;
}

struct LinearHead {
  std::array<double, kFlatFeatureCount> feature_mean{};
  std::array<double, kFlatFeatureCount> feature_scale{};
  std::array<double, kEdgeCount> target_mean{};
  std::array<double, kEdgeCount> target_scale{};
  std::array<std::array<double, kEdgeCount>, kFlatFeatureCount + 1> weights{};
  double minimum_abs_pivot{std::numeric_limits<double>::infinity()};
};

LinearHead fit_linear_head(const Dataset &dataset,
                           const std::vector<int64_t> &indices) {
  require(!indices.empty(), "linear fit range is empty");
  LinearHead model{};
  const double count = static_cast<double>(indices.size());
  for (const int64_t anchor : indices) {
    const auto &features = dataset.edge_features[static_cast<std::size_t>(anchor)];
    const auto &targets = dataset.edge_targets[static_cast<std::size_t>(anchor)];
    for (int64_t feature = 0; feature < kFlatFeatureCount; ++feature) {
      model.feature_mean[static_cast<std::size_t>(feature)] +=
          features[static_cast<std::size_t>(feature)] / count;
    }
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      model.target_mean[static_cast<std::size_t>(edge)] +=
          targets[static_cast<std::size_t>(edge)] / count;
    }
  }
  for (const int64_t anchor : indices) {
    const auto &features = dataset.edge_features[static_cast<std::size_t>(anchor)];
    const auto &targets = dataset.edge_targets[static_cast<std::size_t>(anchor)];
    for (int64_t feature = 0; feature < kFlatFeatureCount; ++feature) {
      const double centered =
          features[static_cast<std::size_t>(feature)] -
          model.feature_mean[static_cast<std::size_t>(feature)];
      model.feature_scale[static_cast<std::size_t>(feature)] +=
          centered * centered / count;
    }
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      const double centered = targets[static_cast<std::size_t>(edge)] -
                              model.target_mean[static_cast<std::size_t>(edge)];
      model.target_scale[static_cast<std::size_t>(edge)] +=
          centered * centered / count;
    }
  }
  for (double &scale : model.feature_scale) {
    scale = std::max(1.0e-12, std::sqrt(scale));
  }
  for (double &scale : model.target_scale) {
    scale = std::max(1.0e-12, std::sqrt(scale));
  }

  constexpr int64_t kDesignWidth = kFlatFeatureCount + 1;
  std::vector<double> gram(static_cast<std::size_t>(kDesignWidth * kDesignWidth),
                           0.0);
  std::vector<double> rhs(static_cast<std::size_t>(kDesignWidth * kEdgeCount),
                          0.0);
  std::array<double, kDesignWidth> design{};
  std::array<double, kEdgeCount> normalized_target{};
  for (const int64_t anchor : indices) {
    const auto &features = dataset.edge_features[static_cast<std::size_t>(anchor)];
    const auto &targets = dataset.edge_targets[static_cast<std::size_t>(anchor)];
    for (int64_t feature = 0; feature < kFlatFeatureCount; ++feature) {
      design[static_cast<std::size_t>(feature)] =
          (features[static_cast<std::size_t>(feature)] -
           model.feature_mean[static_cast<std::size_t>(feature)]) /
          model.feature_scale[static_cast<std::size_t>(feature)];
    }
    design[kFlatFeatureCount] = 1.0;
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      normalized_target[static_cast<std::size_t>(edge)] =
          (targets[static_cast<std::size_t>(edge)] -
           model.target_mean[static_cast<std::size_t>(edge)]) /
          model.target_scale[static_cast<std::size_t>(edge)];
    }
    for (int64_t row = 0; row < kDesignWidth; ++row) {
      for (int64_t column = 0; column < kDesignWidth; ++column) {
        gram[static_cast<std::size_t>(row * kDesignWidth + column)] +=
            design[static_cast<std::size_t>(row)] *
            design[static_cast<std::size_t>(column)];
      }
      for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
        rhs[static_cast<std::size_t>(row * kEdgeCount + edge)] +=
            design[static_cast<std::size_t>(row)] *
            normalized_target[static_cast<std::size_t>(edge)];
      }
    }
  }
  for (int64_t diagonal = 0; diagonal < kFlatFeatureCount; ++diagonal) {
    gram[static_cast<std::size_t>(diagonal * kDesignWidth + diagonal)] +=
        kLinearRidge;
  }

  for (int64_t column = 0; column < kDesignWidth; ++column) {
    int64_t pivot = column;
    double pivot_abs =
        std::abs(gram[static_cast<std::size_t>(column * kDesignWidth + column)]);
    for (int64_t candidate = column + 1; candidate < kDesignWidth; ++candidate) {
      const double candidate_abs = std::abs(
          gram[static_cast<std::size_t>(candidate * kDesignWidth + column)]);
      if (candidate_abs > pivot_abs) {
        pivot_abs = candidate_abs;
        pivot = candidate;
      }
    }
    require(pivot_abs > 1.0e-14 && std::isfinite(pivot_abs),
            "linear normal equation is numerically singular");
    model.minimum_abs_pivot = std::min(model.minimum_abs_pivot, pivot_abs);
    if (pivot != column) {
      for (int64_t entry = 0; entry < kDesignWidth; ++entry) {
        std::swap(gram[static_cast<std::size_t>(column * kDesignWidth + entry)],
                  gram[static_cast<std::size_t>(pivot * kDesignWidth + entry)]);
      }
      for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
        std::swap(rhs[static_cast<std::size_t>(column * kEdgeCount + edge)],
                  rhs[static_cast<std::size_t>(pivot * kEdgeCount + edge)]);
      }
    }
    const double diagonal =
        gram[static_cast<std::size_t>(column * kDesignWidth + column)];
    for (int64_t row = column + 1; row < kDesignWidth; ++row) {
      const double factor =
          gram[static_cast<std::size_t>(row * kDesignWidth + column)] /
          diagonal;
      gram[static_cast<std::size_t>(row * kDesignWidth + column)] = 0.0;
      for (int64_t entry = column + 1; entry < kDesignWidth; ++entry) {
        gram[static_cast<std::size_t>(row * kDesignWidth + entry)] -=
            factor *
            gram[static_cast<std::size_t>(column * kDesignWidth + entry)];
      }
      for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
        rhs[static_cast<std::size_t>(row * kEdgeCount + edge)] -=
            factor * rhs[static_cast<std::size_t>(column * kEdgeCount + edge)];
      }
    }
  }

  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    for (int64_t row = kDesignWidth; row-- > 0;) {
      double value = rhs[static_cast<std::size_t>(row * kEdgeCount + edge)];
      for (int64_t column = row + 1; column < kDesignWidth; ++column) {
        value -= gram[static_cast<std::size_t>(row * kDesignWidth + column)] *
                 model.weights[static_cast<std::size_t>(column)]
                              [static_cast<std::size_t>(edge)];
      }
      model.weights[static_cast<std::size_t>(row)]
                   [static_cast<std::size_t>(edge)] =
          value / gram[static_cast<std::size_t>(row * kDesignWidth + row)];
    }
  }
  return model;
}

std::array<double, kEdgeCount>
predict_linear(const LinearHead &model,
               const std::array<double, kFlatFeatureCount> &features) {
  std::array<double, kEdgeCount> prediction{};
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    double normalized =
        model.weights[kFlatFeatureCount][static_cast<std::size_t>(edge)];
    for (int64_t feature = 0; feature < kFlatFeatureCount; ++feature) {
      normalized +=
          ((features[static_cast<std::size_t>(feature)] -
            model.feature_mean[static_cast<std::size_t>(feature)]) /
           model.feature_scale[static_cast<std::size_t>(feature)]) *
          model.weights[static_cast<std::size_t>(feature)]
                       [static_cast<std::size_t>(edge)];
    }
    prediction[static_cast<std::size_t>(edge)] =
        normalized * model.target_scale[static_cast<std::size_t>(edge)] +
        model.target_mean[static_cast<std::size_t>(edge)];
  }
  return prediction;
}

EdgeMetric evaluate_linear(const LinearHead &model, const Dataset &dataset,
                           const std::vector<int64_t> &indices) {
  EdgeMetric metric{};
  for (const int64_t anchor : indices) {
    const auto &features = dataset.edge_features[static_cast<std::size_t>(anchor)];
    metric.observe(predict_linear(model, features),
                   dataset.edge_targets[static_cast<std::size_t>(anchor)]);
  }
  return metric;
}

using BatchSchedule = std::vector<std::vector<int64_t>>;

BatchSchedule make_schedule(const std::vector<int64_t> &fit_indices) {
  std::mt19937_64 rng(kSeed);
  std::vector<int64_t> order = fit_indices;
  BatchSchedule schedule;
  schedule.reserve(kTrainingSteps);
  while (static_cast<int64_t>(schedule.size()) < kTrainingSteps) {
    std::shuffle(order.begin(), order.end(), rng);
    for (std::size_t offset = 0;
         offset < order.size() &&
         static_cast<int64_t>(schedule.size()) < kTrainingSteps;
         offset += static_cast<std::size_t>(kBatchSize)) {
      const std::size_t end =
          std::min(order.size(), offset + static_cast<std::size_t>(kBatchSize));
      schedule.emplace_back(order.begin() + static_cast<std::ptrdiff_t>(offset),
                            order.begin() + static_cast<std::ptrdiff_t>(end));
    }
  }
  require(static_cast<int64_t>(schedule.size()) == kTrainingSteps,
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

enum class ArmObjective { kNllOnly, kCurrentDirectReadout };

const char *objective_name(ArmObjective objective) {
  return objective == ArmObjective::kNllOnly ? "nll_only"
                                             : "current_direct_readout";
}

mdn::ChannelContextMdn make_model(int64_t mixture_count,
                                  ArmObjective objective,
                                  const torch::Device &device) {
  seed_torch(device);
  mdn::DirectEdgeReturnHeadOptions direct_options{};
  if (objective == ArmObjective::kCurrentDirectReadout) {
    direct_options.identity_mode = "edge_embedding_per_edge";
    direct_options.base_edge_count = kEdgeCount;
    direct_options.identity_embedding_dim = kDirectIdentityEmbeddingDim;
    direct_options.adapter_hidden_dim = kDirectAdapterHiddenDim;
  }
  return mdn::ChannelContextMdn(
      kInputLength, kTargetFeatureCount, kChannelCount, 1, mixture_count,
      kHiddenWidth, kResidualDepth, torch::kFloat32, device,
      kFeatureEmbeddingDim, kChannelAdapterRank,
      std::vector<int64_t>{kRawCloseCoord}, kSigmaFloor, direct_options);
}

mdn::channel_context_mdn_train_options_t nll_only_options() {
  mdn::channel_context_mdn_train_options_t options{};
  options.nll.eps = 1.0e-6;
  options.nll.sigma_min = kSigmaFloor;
  options.nll.sigma_max = 0.0;
  options.grad_clip_norm = kGradClipNorm;
  options.skip_non_finite_loss = false;
  options.edge_return_auxiliary_loss_weight = 0.0;
  options.edge_return_auxiliary_direction_weight = 0.0;
  options.edge_return_auxiliary_rank_weight = 0.0;
  options.direct_edge_return_readout_enabled = false;
  options.direct_edge_return_readout_loss_weight = 0.0;
  options.direct_edge_return_readout_direction_weight = 0.0;
  options.direct_edge_return_readout_rank_weight = 0.0;
  options.direct_edge_return_readout_warmup_steps = 0;
  options.direct_edge_return_readout_warmup_nll_weight = 1.0;
  options.direct_edge_return_readout_post_warmup_nll_weight = 1.0;
  options.direct_edge_return_readout_warmup_direct_head_only = false;
  return options;
}

mdn::channel_context_mdn_train_options_t current_direct_options() {
  auto options = nll_only_options();
  options.direct_edge_return_readout_enabled = true;
  options.direct_edge_return_readout_loss_weight = kDirectRegressionWeight;
  options.direct_edge_return_readout_direction_weight = kDirectDirectionWeight;
  options.direct_edge_return_readout_rank_weight = kDirectRankWeight;
  options.direct_edge_return_readout_huber_beta = kDirectHuberBeta;
  options.direct_edge_return_readout_logit_scale = kDirectLogitScale;
  options.direct_edge_return_readout_target_scale = kDirectTargetScale;
  options.direct_edge_return_readout_warmup_steps = kDirectWarmupSteps;
  options.direct_edge_return_readout_warmup_nll_weight = 0.0;
  options.direct_edge_return_readout_post_warmup_nll_weight = 1.0;
  options.direct_edge_return_readout_warmup_direct_head_only = true;
  return options;
}

void require_nll_only_step(
    const mdn::channel_context_mdn_train_step_result_t &step) {
  require(step.loss.defined() && step.nll.defined(),
          "NLL-only step did not define loss tensors");
  require(torch::equal(step.loss.detach(), step.nll.detach()),
          "NLL-only total loss differs from NLL");
  require(step.edge_return_auxiliary_loss.defined() &&
              step.edge_return_auxiliary_loss.detach()
                      .to(torch::kCPU)
                      .template item<double>() == 0.0,
          "edge auxiliary objective is not neutral");
  require(step.direct_edge_return_readout_loss.defined() &&
              step.direct_edge_return_readout_loss.detach()
                      .to(torch::kCPU)
                      .template item<double>() == 0.0,
          "direct readout objective is not neutral");
  require(step.direct_edge_return_readout_scheduled_nll_weight == 1.0,
          "scheduled NLL weight is not exactly one");
  require(!step.direct_edge_return_readout_warmup_active &&
              !step.direct_edge_return_readout_direct_head_only_warmup_active,
          "direct-readout warmup is unexpectedly active");
}

void require_current_direct_step(
    const mdn::channel_context_mdn_train_step_result_t &step,
    int64_t optimizer_step_index) {
  require(step.loss.defined() && step.nll.defined() &&
              step.direct_edge_return_readout_loss.defined(),
          "direct-readout step did not define objective tensors");
  require(step.edge_return_auxiliary_loss.defined() &&
              step.edge_return_auxiliary_loss.detach()
                      .to(torch::kCPU)
                      .template item<double>() == 0.0,
          "edge auxiliary objective is not neutral");
  require(step.direct_edge_return_readout_loss_valid_count > 0 &&
              step.direct_edge_return_readout_loss_pairwise_valid_count > 0,
          "direct-readout step has no valid edge targets");
  const bool warmup = optimizer_step_index < kDirectWarmupSteps;
  require(step.direct_edge_return_readout_warmup_active == warmup,
          "direct-readout warmup schedule mismatch");
  require(step.direct_edge_return_readout_direct_head_only_warmup_active ==
              warmup,
          "direct-head-only warmup schedule mismatch");
  const double expected_nll_weight = warmup ? 0.0 : 1.0;
  require(step.direct_edge_return_readout_scheduled_nll_weight ==
              expected_nll_weight,
          "scheduled NLL weight mismatch");
  const double total = step.loss.detach().to(torch::kCPU).item<double>();
  const double nll = step.nll.detach().to(torch::kCPU).item<double>();
  const double direct = step.direct_edge_return_readout_loss.detach()
                            .to(torch::kCPU)
                            .item<double>();
  const double expected = expected_nll_weight * nll + direct;
  require(std::isfinite(total) && std::isfinite(nll) &&
              std::isfinite(direct) &&
              std::abs(total - expected) <=
                  1.0e-6 * std::max(1.0, std::abs(expected)),
          "direct-readout scheduled objective identity failed");
}

std::vector<torch::Tensor>
clone_direct_head_parameters(const mdn::ChannelContextMdn &model) {
  std::vector<torch::Tensor> cloned;
  for (const auto &parameter : model->named_parameters(true)) {
    if (parameter.key().find("direct_edge_head") != std::string::npos) {
      cloned.push_back(parameter.value().detach().clone());
    }
  }
  require(!cloned.empty(), "direct edge head parameters are missing");
  return cloned;
}

double direct_head_max_delta(const mdn::ChannelContextMdn &model,
                             const std::vector<torch::Tensor> &before) {
  double maximum = 0.0;
  std::size_t index = 0;
  for (const auto &parameter : model->named_parameters(true)) {
    if (parameter.key().find("direct_edge_head") == std::string::npos) {
      continue;
    }
    require(index < before.size(), "direct edge head parameter count changed");
    maximum = std::max(maximum, (parameter.value().detach() - before[index])
                                    .abs()
                                    .max()
                                    .to(torch::kCPU)
                                    .template item<double>());
    ++index;
  }
  require(index == before.size(), "direct edge head parameter count mismatch");
  return maximum;
}

struct DynamicsPoint {
  int64_t step{0};
  double total_loss{std::numeric_limits<double>::quiet_NaN()};
  double nll{std::numeric_limits<double>::quiet_NaN()};
  double direct_loss{std::numeric_limits<double>::quiet_NaN()};
  double direct_regression_loss{std::numeric_limits<double>::quiet_NaN()};
  double direct_direction_loss{std::numeric_limits<double>::quiet_NaN()};
  double direct_rank_loss{std::numeric_limits<double>::quiet_NaN()};
  double scheduled_nll_weight{std::numeric_limits<double>::quiet_NaN()};
};

struct ArmResult {
  int64_t mixture_count{0};
  ArmObjective objective{ArmObjective::kNllOnly};
  Evaluation canary_initial{};
  Evaluation canary_final{};
  Evaluation train_initial{};
  Evaluation validation_initial{};
  Evaluation train_final{};
  Evaluation validation_final{};
  Evaluation certified_final{};
  std::vector<DynamicsPoint> dynamics{};
  int64_t skipped_steps{0};
  double direct_head_parameter_max_delta{
      std::numeric_limits<double>::quiet_NaN()};
  bool canary_pass{false};
  bool expectation_validation_gate_pass{false};
  bool expectation_certified_gate_pass{false};
  bool direct_validation_gate_pass{false};
  bool direct_certified_gate_pass{false};
  bool primary_validation_gate_pass{false};
  bool primary_certified_gate_pass{false};
  bool overall_pass{false};
};

ArmResult run_arm(int64_t mixture_count, ArmObjective objective,
                  const Dataset &dataset,
                  const std::vector<int64_t> &train_indices,
                  const std::vector<int64_t> &validation_indices,
                  const std::vector<int64_t> &certified_indices,
                  const std::vector<int64_t> &canary_indices,
                  const BatchSchedule &schedule, const torch::Device &device) {
  ArmResult result{};
  result.mixture_count = mixture_count;
  result.objective = objective;
  const auto training_options =
      objective == ArmObjective::kNllOnly ? nll_only_options()
                                          : current_direct_options();

  {
    auto canary_model = make_model(mixture_count, objective, device);
    result.canary_initial =
        evaluate(canary_model, dataset, canary_indices, device, mixture_count);
    mdn::channel_context_mdn_train_model_t canary_train(
        canary_model, kLearningRate, training_options);
    const auto canary_input = make_input(dataset, canary_indices, device);
    for (int64_t step = 0; step < kCanarySteps; ++step) {
      const auto update = canary_train.train_one_batch(canary_input);
      require(!update.skipped && update.optimizer_step_applied,
              "mechanical canary optimizer step failed");
      if (objective == ArmObjective::kNllOnly) {
        require_nll_only_step(update);
      } else {
        require_current_direct_step(update, step);
      }
    }
    result.canary_final = evaluate(canary_train.model(), dataset,
                                   canary_indices, device, mixture_count);
    result.canary_pass = result.canary_final.finite;
    if (objective == ArmObjective::kNllOnly) {
      result.canary_pass =
          result.canary_pass &&
          result.canary_final.mean_nll < result.canary_initial.mean_nll;
    } else {
      result.canary_pass =
          result.canary_pass &&
          result.canary_final.direct_edge.rmse() <
              result.canary_initial.direct_edge.rmse();
    }
  }

  auto model = make_model(mixture_count, objective, device);
  result.train_initial =
      evaluate(model, dataset, train_indices, device, mixture_count);
  result.validation_initial =
      evaluate(model, dataset, validation_indices, device, mixture_count);
  const auto direct_before = clone_direct_head_parameters(model);
  mdn::channel_context_mdn_train_model_t train(model, kLearningRate,
                                               training_options);

  for (int64_t step = 0; step < kTrainingSteps; ++step) {
    const auto input =
        make_input(dataset, schedule[static_cast<std::size_t>(step)], device);
    const auto update = train.train_one_batch(input);
    if (update.skipped) {
      ++result.skipped_steps;
      continue;
    }
    require(update.optimizer_step_applied && update.gradients_finite,
            "main optimizer step failed");
    if (objective == ArmObjective::kNllOnly) {
      require_nll_only_step(update);
    } else {
      require_current_direct_step(update, step);
    }
    const double total_loss =
        update.loss.to(torch::kCPU).template item<double>();
    const double nll = update.nll.to(torch::kCPU).template item<double>();
    const double direct_loss = update.direct_edge_return_readout_loss
                                   .to(torch::kCPU)
                                   .template item<double>();
    if (step == 0 || (step + 1) % 250 == 0 || step + 1 == kTrainingSteps) {
      result.dynamics.push_back(DynamicsPoint{
          step + 1,
          total_loss,
          nll,
          direct_loss,
          update.direct_edge_return_readout_regression_loss
              .to(torch::kCPU)
              .template item<double>(),
          update.direct_edge_return_readout_direction_loss
              .to(torch::kCPU)
              .template item<double>(),
          update.direct_edge_return_readout_rank_loss
              .to(torch::kCPU)
              .template item<double>(),
          update.direct_edge_return_readout_scheduled_nll_weight});
    }
    if ((step + 1) % 500 == 0) {
      std::cerr << "arm.k" << mixture_count << " step=" << (step + 1)
                << " total=" << std::setprecision(10) << total_loss
                << " nll=" << nll << " direct=" << direct_loss << '\n';
    }
  }

  require(result.skipped_steps == 0,
          "main training skipped an optimizer step");
  result.direct_head_parameter_max_delta =
      direct_head_max_delta(train.model(), direct_before);
  result.train_final =
      evaluate(train.model(), dataset, train_indices, device, mixture_count);
  result.validation_final = evaluate(train.model(), dataset, validation_indices,
                                     device, mixture_count);
  // The certified range is scored only here, after the arm is fully trained.
  result.certified_final = evaluate(train.model(), dataset, certified_indices,
                                    device, mixture_count);
  result.expectation_validation_gate_pass =
      result.validation_final.finite &&
      capability_gate(result.validation_final.expectation_edge);
  result.expectation_certified_gate_pass =
      result.certified_final.finite &&
      capability_gate(result.certified_final.expectation_edge);
  result.direct_validation_gate_pass =
      result.validation_final.finite &&
      capability_gate(result.validation_final.direct_edge);
  result.direct_certified_gate_pass =
      result.certified_final.finite &&
      capability_gate(result.certified_final.direct_edge);
  if (objective == ArmObjective::kNllOnly) {
    result.primary_validation_gate_pass =
        result.expectation_validation_gate_pass;
    result.primary_certified_gate_pass =
        result.expectation_certified_gate_pass;
  } else {
    result.primary_validation_gate_pass = result.direct_validation_gate_pass;
    result.primary_certified_gate_pass = result.direct_certified_gate_pass;
  }
  const bool direct_parameter_contract =
      objective == ArmObjective::kNllOnly
          ? result.direct_head_parameter_max_delta == 0.0
          : result.direct_head_parameter_max_delta > 0.0;
  result.overall_pass =
      result.canary_pass && result.train_final.finite &&
      result.primary_validation_gate_pass &&
      result.primary_certified_gate_pass && direct_parameter_contract;
  return result;
}

void emit_edge_metric(std::ostream &out, const std::string &prefix,
                      const EdgeMetric &metric) {
  out << prefix << ".count=" << metric.count << '\n';
  out << prefix << ".mae=" << metric.mae() << '\n';
  out << prefix << ".rmse=" << metric.rmse() << '\n';
  out << prefix << ".target_rms=" << metric.target_rms() << '\n';
  out << prefix << ".prediction_rms=" << metric.prediction_rms() << '\n';
  out << prefix << ".rmse_to_target_rms=" << metric.rmse_to_target_rms()
      << '\n';
  out << prefix << ".directional_accuracy=" << metric.direction() << '\n';
  out << prefix << ".pairwise_rank_count=" << metric.pair_count << '\n';
  out << prefix << ".pairwise_rank_accuracy=" << metric.rank() << '\n';
  out << prefix << ".best_asset_agreement=" << metric.best() << '\n';
  out << prefix << ".correlation=" << metric.correlation() << '\n';
  out << prefix << ".finite=" << bool_string(metric.finite()) << '\n';
  out << prefix << ".capability_gate_pass="
      << bool_string(capability_gate(metric)) << '\n';
}

void emit_evaluation(std::ostream &out, const std::string &prefix,
                     const Evaluation &evaluation) {
  emit_edge_metric(out, prefix + ".expectation", evaluation.expectation_edge);
  emit_edge_metric(out, prefix + ".direct", evaluation.direct_edge);
  out << prefix << ".mean_nll=" << evaluation.mean_nll << '\n';
  out << prefix << ".sigma_min=" << evaluation.sigma_min << '\n';
  out << prefix << ".sigma_mean=" << evaluation.sigma_mean << '\n';
  out << prefix << ".sigma_max=" << evaluation.sigma_max << '\n';
  out << prefix << ".mixture_entropy=" << evaluation.mixture_entropy << '\n';
  for (std::size_t component = 0; component < evaluation.mixture_usage.size();
       ++component) {
    out << prefix << ".mixture_usage.k" << component << '='
        << evaluation.mixture_usage[component] << '\n';
  }
  out << prefix << ".evaluation_finite=" << bool_string(evaluation.finite)
      << '\n';
}

void emit_arm(std::ostream &out, const ArmResult &arm) {
  const std::string prefix = "arm.k" + std::to_string(arm.mixture_count);
  out << prefix << ".objective=" << objective_name(arm.objective) << '\n';
  out << prefix << ".primary_output="
      << (arm.objective == ArmObjective::kNllOnly ? "mdn_expectation"
                                                  : "direct_edge_return")
      << '\n';
  emit_evaluation(out, prefix + ".canary.initial", arm.canary_initial);
  emit_evaluation(out, prefix + ".canary.final", arm.canary_final);
  emit_evaluation(out, prefix + ".initial.train", arm.train_initial);
  emit_evaluation(out, prefix + ".initial.validation", arm.validation_initial);
  emit_evaluation(out, prefix + ".final.train", arm.train_final);
  emit_evaluation(out, prefix + ".final.validation", arm.validation_final);
  emit_evaluation(out, prefix + ".final.certified", arm.certified_final);
  for (std::size_t index = 0; index < arm.dynamics.size(); ++index) {
    out << prefix << ".training.point." << std::setw(2) << std::setfill('0')
        << index << std::setfill(' ') << ".step=" << arm.dynamics[index].step
        << '\n';
    const std::string point =
        prefix + ".training.point." +
        (index < 10 ? "0" : "") + std::to_string(index);
    out << point << ".total_loss=" << arm.dynamics[index].total_loss << '\n';
    out << point << ".nll=" << arm.dynamics[index].nll << '\n';
    out << point << ".direct_loss=" << arm.dynamics[index].direct_loss
        << '\n';
    out << point << ".direct_regression_loss="
        << arm.dynamics[index].direct_regression_loss << '\n';
    out << point << ".direct_direction_loss="
        << arm.dynamics[index].direct_direction_loss << '\n';
    out << point << ".direct_rank_loss="
        << arm.dynamics[index].direct_rank_loss << '\n';
    out << point << ".scheduled_nll_weight="
        << arm.dynamics[index].scheduled_nll_weight << '\n';
  }
  out << prefix << ".skipped_steps=" << arm.skipped_steps << '\n';
  out << prefix << ".direct_head_parameter_max_abs_delta="
      << arm.direct_head_parameter_max_delta << '\n';
  out << prefix << ".canary_pass=" << bool_string(arm.canary_pass) << '\n';
  out << prefix << ".expectation_validation_gate_pass="
      << bool_string(arm.expectation_validation_gate_pass) << '\n';
  out << prefix << ".expectation_certified_gate_pass="
      << bool_string(arm.expectation_certified_gate_pass) << '\n';
  out << prefix << ".direct_validation_gate_pass="
      << bool_string(arm.direct_validation_gate_pass) << '\n';
  out << prefix << ".direct_certified_gate_pass="
      << bool_string(arm.direct_certified_gate_pass) << '\n';
  out << prefix << ".primary_validation_gate_pass="
      << bool_string(arm.primary_validation_gate_pass) << '\n';
  out << prefix << ".primary_certified_gate_pass="
      << bool_string(arm.primary_certified_gate_pass) << '\n';
  out << prefix << ".overall_pass=" << bool_string(arm.overall_pass) << '\n';
}

void write_report(const Options &options, const Dataset &dataset,
                  const std::string &schedule_sha, const torch::Device &device,
                  const LinearHead &linear, const EdgeMetric &linear_train,
                  const EdgeMetric &linear_validation,
                  const EdgeMetric &linear_certified, const ArmResult &k1,
                  const ArmResult &k3) {
  std::filesystem::create_directories(options.output.parent_path());
  std::ofstream out(options.output, std::ios::trunc);
  require(out.good(), "cannot open output report");
  out << std::setprecision(12) << std::fixed;
  out << "schema_id=" << options.schema_id << '\n';
  out << "status=complete\n";
  out << "benchmark_id=synthetic_continuous_graph_v2\n";
  out << "diagnostic_authority=development_only\n";
  out << "benchmark_acceptance_authority=false\n";
  out << "question=compare_production_k1_nll_only_and_k3_current_direct_"
         "readout_on_29_raw_causal_daily_return_lags_without_representation\n";
  out << '\n';
  out << "provenance.policy_path_used=false\n";
  out << "provenance.representation_forward_executed=false\n";
  out << "provenance.representation_checkpoint_used=false\n";
  out << "provenance.production_mdn_checkpoint_used=false\n";
  out << "provenance.production_channel_context_mdn_executed=true\n";
  out << "provenance.production_mdn_train_model_executed=true\n";
  out << "provenance.k1_mdn_nll_only_optimized=true\n";
  out << "provenance.k3_current_direct_readout_optimized=true\n";
  out << "provenance.checkpoint_written=false\n";
  out << "provenance.full_data_raw_path_opened=false\n";
  out << "provenance.final_holdout_input_used=false\n";
  out << "provenance.completed_data_closure_required=true\n";
  out << '\n';
  out << "data.interval=1d\n";
  out << "data.normalization=previous_close_log_return\n";
  out << "data.context_definition=29_causal_daily_log_return_lags_per_3_edges\n";
  out << "data.target_definition=next_daily_log_return_per_3_edges\n";
  out << "data.context_shape=[B,4,1,29]\n";
  out << "data.future_shape=[B,4,1,1]\n";
  out << "data.node_order=SYN2USD,SYN2ALPHA,SYN2BETA,SYN2GAMMA\n";
  out << "data.prefix_row_count=" << kRequiredPrefixRows << '\n';
  out << "data.alpha.prefix_sha256=" << dataset.prefix_sha256[0] << '\n';
  out << "data.beta.prefix_sha256=" << dataset.prefix_sha256[1] << '\n';
  out << "data.gamma.prefix_sha256=" << dataset.prefix_sha256[2] << '\n';
  out << "data.closure_sha256=" << dataset.closure_sha256 << '\n';
  out << "data.input_combined_sha256="
      << digest::sha256_hex(dataset.prefix_sha256[0] +
                            dataset.prefix_sha256[1] +
                            dataset.prefix_sha256[2] + dataset.closure_sha256)
      << '\n';
  out << "data.cross_instrument_timestamp_alignment_passed=true\n";
  out << "data.strict_1d_cadence_passed=true\n";
  out << "data.projection_max_abs_error=" << dataset.projection_max_abs_error
      << '\n';
  out << "data.float_projection_max_abs_error="
      << dataset.float_projection_max_abs_error << '\n';
  out << "max_anchor_read=" << kMaximumAnchorLoaded << '\n';
  out << "max_daily_row_read=" << kMaximumSourceRowLoaded << '\n';
  out << "test_input_used=false\n";
  out << "test_boundary_assertion_passed=true\n";
  out << '\n';
  out << "boundary.declared_train=[" << kDeclaredTrainBegin << ','
      << kTrainEnd << ")\n";
  out << "boundary.effective_train=[" << kEffectiveTrainBegin << ','
      << kTrainEnd << ")\n";
  out << "boundary.purge_1=[2496,2560)\n";
  out << "boundary.validation=[" << kValidationBegin << ',' << kValidationEnd
      << ")\n";
  out << "boundary.purge_2=[2816,2880)\n";
  out << "boundary.certified_eval=[" << kCertifiedBegin << ','
      << kCertifiedEnd << ")\n";
  out << "boundary.purge_3=[3264,3328)\n";
  out << "boundary.final_holdout=[3328,4096)\n";
  out << "boundary.purge_ranges_opened=false\n";
  out << "boundary.final_holdout_opened=false\n";
  out << "boundary.certified_scored_after_training_only=true\n";
  out << "boundary.certified_refit_after_scoring=false\n";
  out << '\n';
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
  out << "execution.schedule_sha256=" << schedule_sha << '\n';
  out << "execution.k1_k3_schedule_identical=true\n";
  out << "execution.k1_objective=nll_only\n";
  out << "execution.k3_objective=current_direct_readout\n";
  out << "execution.k3.direct_regression_weight="
      << kDirectRegressionWeight << '\n';
  out << "execution.k3.direct_direction_weight=" << kDirectDirectionWeight
      << '\n';
  out << "execution.k3.direct_rank_weight=" << kDirectRankWeight << '\n';
  out << "execution.k3.direct_huber_beta=" << kDirectHuberBeta << '\n';
  out << "execution.k3.direct_logit_scale=" << kDirectLogitScale << '\n';
  out << "execution.k3.direct_target_scale=" << kDirectTargetScale << '\n';
  out << "execution.k3.direct_warmup_steps=" << kDirectWarmupSteps << '\n';
  out << "execution.k3.direct_warmup_nll_weight=0.000000000000\n";
  out << "execution.k3.direct_post_warmup_nll_weight=1.000000000000\n";
  out << "execution.k3.direct_warmup_direct_head_only=true\n";
  out << "execution.k3.direct_identity_mode=edge_embedding_per_edge\n";
  out << "execution.k3.direct_base_edge_count=" << kEdgeCount << '\n';
  out << "execution.k3.direct_identity_embedding_dim="
      << kDirectIdentityEmbeddingDim << '\n';
  out << "execution.k3.direct_adapter_hidden_dim="
      << kDirectAdapterHiddenDim << '\n';
  out << "execution.main_replay_contract=byte_identical_reports_and_logs\n";
  out << '\n';
  out << "gate.direction_threshold=" << kDirectionGate << '\n';
  out << "gate.pairwise_rank_threshold=" << kRankGate << '\n';
  out << "gate.correlation_threshold=" << kCorrelationGate << '\n';
  out << "gate.rmse_to_target_rms_maximum=" << kRmseRatioGate << '\n';
  out << "gate.failure_is_nonfatal=true\n";
  out << '\n';
  out << "linear.ridge=" << kLinearRidge << '\n';
  out << "linear.minimum_abs_elimination_pivot=" << linear.minimum_abs_pivot
      << '\n';
  emit_edge_metric(out, "linear.train", linear_train);
  emit_edge_metric(out, "linear.validation", linear_validation);
  emit_edge_metric(out, "linear.certified", linear_certified);
  const bool linear_pass = capability_gate(linear_validation) &&
                           capability_gate(linear_certified);
  out << "linear.overall_pass=" << bool_string(linear_pass) << '\n';
  out << '\n';
  emit_arm(out, k1);
  out << '\n';
  emit_arm(out, k3);
  out << '\n';
  const bool neural_pass = k1.overall_pass || k3.overall_pass;
  out << "classification.linear_control_pass=" << bool_string(linear_pass)
      << '\n';
  out << "classification.mdn_k1_pass=" << bool_string(k1.overall_pass) << '\n';
  out << "classification.mdn_k3_pass=" << bool_string(k3.overall_pass) << '\n';
  out << "classification.mdn_k1_primary_output=mdn_expectation\n";
  out << "classification.mdn_k3_primary_output=direct_edge_return\n";
  out << "classification.supervised_raw_history_neural_pass="
      << bool_string(neural_pass) << '\n';
  out << "raw_history_supervised_gate_passed=" << bool_string(neural_pass)
      << '\n';
  std::string classification;
  if (k1.overall_pass && k3.overall_pass) {
    classification = "k1_expectation_and_k3_direct_raw_history_pass";
  } else if (k1.overall_pass && !k3.overall_pass) {
    classification =
        "k1_expectation_pass_k3_current_direct_training_fail";
  } else if (!k1.overall_pass && k3.overall_pass) {
    classification = "k1_expectation_fail_k3_current_direct_readout_pass";
  } else {
    classification = "k1_expectation_and_k3_direct_raw_history_fail";
  }
  out << "classification.result=" << classification << '\n';
  out << "classification.causal_attribution="
         "not_established_by_single_seed_development_diagnostic\n";
  out << "summary=" << classification
      << "_without_representation_policy_or_checkpoint\n";
  require(out.good(), "failed while writing output report");
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    validate_inputs(options);
    at::globalContext().setBenchmarkCuDNN(false);
    at::globalContext().setDeterministicCuDNN(true);
    at::globalContext().setDeterministicAlgorithms(true, false);
    at::globalContext().setDeterministicFillUninitializedMemory(true);

    const torch::Device device =
        !options.force_cpu && torch::cuda::is_available()
            ? torch::Device(torch::kCUDA)
            : torch::Device(torch::kCPU);
    const auto dataset = build_dataset(options);
    const auto train_indices =
        range_indices(kEffectiveTrainBegin, kTrainEnd);
    const auto validation_indices =
        range_indices(kValidationBegin, kValidationEnd);
    const auto certified_indices =
        range_indices(kCertifiedBegin, kCertifiedEnd);
    const auto canary_indices = range_indices(kCanaryBegin, kCanaryEnd);
    const auto schedule = make_schedule(train_indices);

    const auto contract_input =
        make_input(dataset, std::vector<int64_t>{1, 2}, device);
    require(contract_input.context.sizes() ==
                torch::IntArrayRef({2, kNodeCount, 1, kInputLength}),
            "MDN adapter context contract failed");
    require(contract_input.future.sizes() ==
                torch::IntArrayRef({2, kNodeCount, 1, 1}),
            "MDN adapter future contract failed");
    require(contract_input.target_coords ==
                std::vector<int64_t>{kRawCloseCoord},
            "MDN adapter target coordinate identity failed");

    const auto linear = fit_linear_head(dataset, train_indices);
    const auto linear_train =
        evaluate_linear(linear, dataset, train_indices);
    const auto linear_validation =
        evaluate_linear(linear, dataset, validation_indices);
    const auto linear_certified =
        evaluate_linear(linear, dataset, certified_indices);

    std::cerr << "running production raw-history MDN K=1 NLL-only on "
              << (device.is_cuda() ? "cuda" : "cpu") << '\n';
    const auto k1 =
        run_arm(1, ArmObjective::kNllOnly, dataset, train_indices,
                validation_indices, certified_indices, canary_indices,
                schedule, device);
    std::cerr << "running production raw-history MDN K=3 current direct "
                 "readout on "
              << (device.is_cuda() ? "cuda" : "cpu") << '\n';
    const auto k3 =
        run_arm(3, ArmObjective::kCurrentDirectReadout, dataset, train_indices,
                validation_indices, certified_indices, canary_indices,
                schedule, device);

    write_report(options, dataset, schedule_digest(schedule), device, linear,
                 linear_train, linear_validation, linear_certified, k1, k3);
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "error: " << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << '\n';
  }
  return 1;
}
