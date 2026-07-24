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

constexpr int64_t kRequiredPrefixRows = 760;
constexpr int64_t kInputLength = 30;
constexpr int64_t kDeclaredFitBegin = 0;
constexpr int64_t kEffectiveFitBegin = 1;
constexpr int64_t kFitEnd = 554;
constexpr int64_t kPurgeEnd = 584;
constexpr int64_t kValidationEnd = 730;
constexpr int64_t kMaximumAnchorLoaded = kValidationEnd - 1;
constexpr int64_t kMaximumSourceRowLoaded = kMaximumAnchorLoaded + kInputLength;
constexpr int64_t kNodeCount = 4;
constexpr int64_t kChannelCount = 1;
constexpr int64_t kTargetFeatureCount = 1;
constexpr int64_t kRawCloseCoord = 3;
constexpr int64_t kKlineFeatureWidth = 9;
constexpr int64_t kHiddenWidth = 128;
constexpr int64_t kResidualDepth = 2;
constexpr int64_t kFeatureEmbeddingDim = 8;
constexpr int64_t kChannelAdapterRank = 16;
constexpr int64_t kTrainingSteps = 3500;
constexpr int64_t kBatchSize = 64;
constexpr int64_t kCanaryBegin = 1;
constexpr int64_t kCanaryEnd = 17;
constexpr int64_t kCanarySteps = 1000;
constexpr int64_t kSeed = 31;
constexpr double kLearningRate = 1.0e-3;
constexpr double kGradClipNorm = 5.0;
constexpr double kSigmaFloor = 1.0e-3;
constexpr double kMarginEps = 1.0e-3;
constexpr double kDirectionGate = 0.95;
constexpr double kRankGate = 0.95;

struct Options {
  std::filesystem::path alpha_prefix;
  std::filesystem::path beta_prefix;
  std::filesystem::path gamma_prefix;
  std::filesystem::path output;
  std::string schema_id{"synthetic_mdn_raw_history_isolation_v1"};
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
  if (index + 1 >= argc) {
    fail("missing value for " + flag);
  }
  return argv[++index];
}

Options parse_options(int argc, char **argv) {
  Options options{};
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--alpha-prefix") {
      options.alpha_prefix = required_value(argc, argv, i, arg);
    } else if (arg == "--beta-prefix") {
      options.beta_prefix = required_value(argc, argv, i, arg);
    } else if (arg == "--gamma-prefix") {
      options.gamma_prefix = required_value(argc, argv, i, arg);
    } else if (arg == "--output") {
      options.output = required_value(argc, argv, i, arg);
    } else if (arg == "--schema-id") {
      options.schema_id = required_value(argc, argv, i, arg);
    } else if (arg == "--cpu") {
      options.force_cpu = true;
    } else {
      fail("unknown argument: " + arg);
    }
  }
  require(!options.alpha_prefix.empty(), "--alpha-prefix is required");
  require(!options.beta_prefix.empty(), "--beta-prefix is required");
  require(!options.gamma_prefix.empty(), "--gamma-prefix is required");
  require(!options.output.empty(), "--output is required");
  require(options.schema_id == "synthetic_mdn_raw_history_isolation_v1",
          "unexpected schema id");
  return options;
}

std::string read_file(const std::filesystem::path &path) {
  std::ifstream in(path, std::ios::binary);
  require(in.good(), "cannot open input: " + path.string());
  std::ostringstream buffer;
  buffer << in.rdbuf();
  require(in.good() || in.eof(), "cannot read input: " + path.string());
  return buffer.str();
}

std::string sha256_file(const std::filesystem::path &path) {
  return digest::sha256_hex(read_file(path));
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
    if (!std::getline(row, fields[field], ',')) {
      fail("malformed kline row " + std::to_string(row_index));
    }
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
  std::ifstream in(path);
  require(in.good(), "cannot open prefix: " + path.string());
  std::vector<KlineRow> rows;
  rows.reserve(kRequiredPrefixRows);
  std::string line;
  while (static_cast<int64_t>(rows.size()) < kRequiredPrefixRows &&
         std::getline(in, line)) {
    rows.push_back(parse_kline_row(line, static_cast<int64_t>(rows.size())));
  }
  require(static_cast<int64_t>(rows.size()) == kRequiredPrefixRows,
          "prefix must contain exactly the required first 760 rows");
  return rows;
}

std::array<double, kNodeCount>
uniform_gauge_lift(const std::array<double, 3> &edge_returns) {
  const double quote =
      -(edge_returns[0] + edge_returns[1] + edge_returns[2]) / 4.0;
  return {quote, edge_returns[0] + quote, edge_returns[1] + quote,
          edge_returns[2] + quote};
}

struct Dataset {
  torch::Tensor context;              // [A,4,1,30]
  torch::Tensor context_mask;         // [A,4,1]
  torch::Tensor future_node_features; // [A,1,1,4,9]
  torch::Tensor future_node_mask;     // [A,1,1,4,9]
  torch::Tensor anchor_keys;          // [A]
  std::array<std::string, 3> prefix_sha256{};
  double projection_max_abs_error{0.0};
  double float_projection_max_abs_error{0.0};
};

Dataset build_dataset(const Options &options) {
  constexpr int64_t kOneDayMs = 86400000;
  const std::array<std::vector<KlineRow>, 3> rows{
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
              "1d open-time cadence is not strictly monotonic at prefix row " +
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
  dataset.context =
      torch::zeros({kValidationEnd, kNodeCount, kChannelCount, kInputLength},
                   torch::kFloat32);
  dataset.context_mask =
      torch::ones({kValidationEnd, kNodeCount, kChannelCount}, torch::kBool);
  dataset.future_node_features = torch::zeros(
      {kValidationEnd, kChannelCount, 1, kNodeCount, kKlineFeatureWidth},
      torch::kFloat32);
  dataset.future_node_mask = torch::zeros(
      {kValidationEnd, kChannelCount, 1, kNodeCount, kKlineFeatureWidth},
      torch::kBool);
  dataset.anchor_keys = torch::empty(
      {kValidationEnd}, torch::TensorOptions().dtype(torch::kInt64));

  auto context = dataset.context.accessor<float, 4>();
  auto context_mask = dataset.context_mask.accessor<bool, 3>();
  auto future = dataset.future_node_features.accessor<float, 5>();
  auto future_mask = dataset.future_node_mask.accessor<bool, 5>();
  auto anchor_keys = dataset.anchor_keys.accessor<int64_t, 1>();

  for (int64_t anchor = 0; anchor < kValidationEnd; ++anchor) {
    anchor_keys[anchor] =
        rows[0][static_cast<std::size_t>(anchor + kInputLength - 1)].close_time;
    if (anchor == 0) {
      for (int64_t node = 0; node < kNodeCount; ++node) {
        context_mask[anchor][node][0] = false;
      }
    }
    for (int64_t history = 0; history < kInputLength; ++history) {
      const int64_t row = anchor + history;
      if (row == 0) {
        continue;
      }
      std::array<double, 3> edges{};
      for (std::size_t edge = 0; edge < edges.size(); ++edge) {
        edges[edge] =
            std::log(rows[edge][static_cast<std::size_t>(row)].close /
                     rows[edge][static_cast<std::size_t>(row - 1)].close);
      }
      const auto nodes = uniform_gauge_lift(edges);
      for (int64_t node = 0; node < kNodeCount; ++node) {
        context[anchor][node][0][history] =
            static_cast<float>(nodes[static_cast<std::size_t>(node)]);
      }
      for (int64_t edge = 0; edge < 3; ++edge) {
        const double reconstructed =
            nodes[static_cast<std::size_t>(edge + 1)] - nodes[0];
        dataset.projection_max_abs_error = std::max(
            dataset.projection_max_abs_error,
            std::abs(reconstructed - edges[static_cast<std::size_t>(edge)]));
        const double reconstructed_float =
            static_cast<double>(context[anchor][edge + 1][0][history]) -
            static_cast<double>(context[anchor][0][0][history]);
        dataset.float_projection_max_abs_error =
            std::max(dataset.float_projection_max_abs_error,
                     std::abs(reconstructed_float -
                              edges[static_cast<std::size_t>(edge)]));
      }
    }

    const int64_t target_row = anchor + kInputLength;
    std::array<double, 3> target_edges{};
    for (std::size_t edge = 0; edge < target_edges.size(); ++edge) {
      target_edges[edge] =
          std::log(rows[edge][static_cast<std::size_t>(target_row)].close /
                   rows[edge][static_cast<std::size_t>(target_row - 1)].close);
    }
    const auto target_nodes = uniform_gauge_lift(target_edges);
    for (int64_t node = 0; node < kNodeCount; ++node) {
      future[anchor][0][0][node][kRawCloseCoord] =
          static_cast<float>(target_nodes[static_cast<std::size_t>(node)]);
      future_mask[anchor][0][0][node][kRawCloseCoord] = true;
    }
    for (int64_t edge = 0; edge < 3; ++edge) {
      const double reconstructed =
          target_nodes[static_cast<std::size_t>(edge + 1)] - target_nodes[0];
      dataset.projection_max_abs_error =
          std::max(dataset.projection_max_abs_error,
                   std::abs(reconstructed -
                            target_edges[static_cast<std::size_t>(edge)]));
    }
  }

  require(dataset.context.sizes() ==
              torch::IntArrayRef(
                  {kValidationEnd, kNodeCount, kChannelCount, kInputLength}),
          "raw context shape mismatch");
  require(dataset.context_mask.sizes() ==
              torch::IntArrayRef({kValidationEnd, kNodeCount, kChannelCount}),
          "raw context mask shape mismatch");
  require(dataset.future_node_features.sizes() ==
              torch::IntArrayRef({kValidationEnd, kChannelCount, 1, kNodeCount,
                                  kKlineFeatureWidth}),
          "future source shape mismatch");
  require(dataset.projection_max_abs_error <= 1.0e-12,
          "uniform-gauge projection identity failed");
  require(dataset.float_projection_max_abs_error <= 1.0e-7,
          "float uniform-gauge projection drift is too large");
  require(kMaximumSourceRowLoaded == 759,
          "maximum source-row boundary changed");
  return dataset;
}

std::vector<int64_t> range_indices(int64_t begin, int64_t end) {
  require(begin >= 0 && end >= begin && end <= kValidationEnd,
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
  const auto context = dataset.context.index_select(0, index).to(device);
  const auto context_mask =
      dataset.context_mask.index_select(0, index).to(device);
  const auto future =
      dataset.future_node_features.index_select(0, index).to(device);
  const auto future_mask =
      dataset.future_node_mask.index_select(0, index).to(device);
  const auto anchor_keys =
      dataset.anchor_keys.index_select(0, index).to(device);
  return mdn::make_channel_mdn_input(
      context, context_mask, future, future_mask,
      std::vector<int64_t>{kRawCloseCoord}, anchor_keys,
      std::vector<std::string>{"SYNUSD", "SYNALPHA", "SYNBETA", "SYNGAMMA"});
}

int sign_of(double value) { return value > 0.0 ? 1 : (value < 0.0 ? -1 : 0); }

struct EdgeMetric {
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
  double abs_error_sum{0.0};
  double squared_error_sum{0.0};
  double prediction_sum{0.0};
  double target_sum{0.0};
  double prediction_squared_sum{0.0};
  double target_squared_sum{0.0};
  double cross_sum{0.0};

  void observe(const std::array<double, 3> &prediction,
               const std::array<double, 3> &target) {
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
      if (std::abs(y) > kMarginEps) {
        ++margin_count;
        if (sign_of(p) == sign_of(y)) {
          ++margin_direction_correct;
        }
      }
    }
    for (std::size_t lhs = 0; lhs < prediction.size(); ++lhs) {
      for (std::size_t rhs = lhs + 1; rhs < prediction.size(); ++rhs) {
        const double pd = prediction[lhs] - prediction[rhs];
        const double yd = target[lhs] - target[rhs];
        ++pair_count;
        if (sign_of(pd) == sign_of(yd)) {
          ++pair_correct;
        }
        if (std::abs(yd) > kMarginEps) {
          ++margin_pair_count;
          if (sign_of(pd) == sign_of(yd)) {
            ++margin_pair_correct;
          }
        }
      }
    }
    const auto predicted_best = static_cast<int64_t>(
        std::distance(prediction.begin(),
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
  double direction() const {
    return count > 0 ? static_cast<double>(direction_correct) /
                           static_cast<double>(count)
                     : std::numeric_limits<double>::quiet_NaN();
  }
  double margin_direction() const {
    return margin_count > 0 ? static_cast<double>(margin_direction_correct) /
                                  static_cast<double>(margin_count)
                            : std::numeric_limits<double>::quiet_NaN();
  }
  double rank() const {
    return pair_count > 0 ? static_cast<double>(pair_correct) /
                                static_cast<double>(pair_count)
                          : std::numeric_limits<double>::quiet_NaN();
  }
  double margin_rank() const {
    return margin_pair_count > 0 ? static_cast<double>(margin_pair_correct) /
                                       static_cast<double>(margin_pair_count)
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
  double prediction_std() const {
    if (count <= 0) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    const double n = static_cast<double>(count);
    return std::sqrt(
        std::max(0.0, prediction_squared_sum / n -
                          (prediction_sum / n) * (prediction_sum / n)));
  }
  double target_std() const {
    if (count <= 0) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    const double n = static_cast<double>(count);
    return std::sqrt(std::max(0.0, target_squared_sum / n -
                                       (target_sum / n) * (target_sum / n)));
  }
};

struct Evaluation {
  EdgeMetric edge{};
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
  evaluation.mixture_usage.reserve(static_cast<std::size_t>(mixture_count));
  for (int64_t component = 0; component < mixture_count; ++component) {
    evaluation.mixture_usage.push_back(usage_accessor[component]);
  }

  const auto expected = mdn::mdn_expectation(output)
                            .to(torch::kCPU)
                            .to(torch::kFloat64)
                            .contiguous();
  const auto realized =
      input.future.to(torch::kCPU).to(torch::kFloat64).contiguous();
  const auto valid = combined_mask.to(torch::kCPU).contiguous();
  const auto expected_a = expected.accessor<double, 4>();
  const auto realized_a = realized.accessor<double, 4>();
  const auto valid_a = valid.accessor<bool, 4>();
  for (int64_t sample = 0; sample < expected.size(0); ++sample) {
    std::array<double, 3> prediction{};
    std::array<double, 3> target{};
    require(valid_a[sample][0][0][0], "quote target is unexpectedly invalid");
    for (int64_t edge = 0; edge < 3; ++edge) {
      require(valid_a[sample][edge + 1][0][0],
              "base target is unexpectedly invalid");
      prediction[static_cast<std::size_t>(edge)] =
          expected_a[sample][edge + 1][0][0] - expected_a[sample][0][0][0];
      target[static_cast<std::size_t>(edge)] =
          realized_a[sample][edge + 1][0][0] - realized_a[sample][0][0][0];
    }
    evaluation.edge.observe(prediction, target);
  }

  evaluation.finite =
      std::isfinite(evaluation.mean_nll) &&
      std::isfinite(evaluation.sigma_min) &&
      std::isfinite(evaluation.sigma_mean) &&
      std::isfinite(evaluation.sigma_max) &&
      std::isfinite(evaluation.mixture_entropy) &&
      evaluation.edge.count == static_cast<int64_t>(indices.size()) * 3;
  return evaluation;
}

EdgeMetric evaluate_control(const Dataset &dataset,
                            const std::vector<int64_t> &indices,
                            bool seasonal_lag) {
  const auto context = dataset.context.accessor<float, 4>();
  const auto future = dataset.future_node_features.accessor<float, 5>();
  const std::array<int64_t, 3> period{12, 10, 8};
  EdgeMetric metric{};
  for (const int64_t anchor : indices) {
    std::array<double, 3> prediction{};
    std::array<double, 3> target{};
    for (int64_t edge = 0; edge < 3; ++edge) {
      const int64_t history =
          seasonal_lag ? kInputLength - period[static_cast<std::size_t>(edge)]
                       : kInputLength - 1;
      prediction[static_cast<std::size_t>(edge)] =
          static_cast<double>(context[anchor][edge + 1][0][history]) -
          static_cast<double>(context[anchor][0][0][history]);
      target[static_cast<std::size_t>(edge)] =
          static_cast<double>(future[anchor][0][0][edge + 1][kRawCloseCoord]) -
          static_cast<double>(future[anchor][0][0][0][kRawCloseCoord]);
    }
    metric.observe(prediction, target);
  }
  return metric;
}

struct GaussianBaseline {
  std::array<double, kNodeCount> mean{};
  std::array<double, kNodeCount> sigma{};
};

GaussianBaseline
fit_unconditional_gaussian(const Dataset &dataset,
                           const std::vector<int64_t> &fit_indices) {
  const auto future = dataset.future_node_features.accessor<float, 5>();
  GaussianBaseline baseline{};
  for (int64_t node = 0; node < kNodeCount; ++node) {
    double sum = 0.0;
    double squared_sum = 0.0;
    for (const int64_t anchor : fit_indices) {
      const double value = future[anchor][0][0][node][kRawCloseCoord];
      sum += value;
      squared_sum += value * value;
    }
    const double count = static_cast<double>(fit_indices.size());
    baseline.mean[static_cast<std::size_t>(node)] = sum / count;
    const double variance =
        std::max(0.0, squared_sum / count - (sum / count) * (sum / count));
    baseline.sigma[static_cast<std::size_t>(node)] =
        std::max(kSigmaFloor, std::sqrt(variance));
  }
  return baseline;
}

double unconditional_gaussian_nll(const Dataset &dataset,
                                  const std::vector<int64_t> &indices,
                                  const GaussianBaseline &baseline) {
  constexpr double kLogTwoPi = 1.8378770664093453;
  const auto future = dataset.future_node_features.accessor<float, 5>();
  double sum = 0.0;
  int64_t count = 0;
  for (const int64_t anchor : indices) {
    for (int64_t node = 0; node < kNodeCount; ++node) {
      const double target = future[anchor][0][0][node][kRawCloseCoord];
      const double mean = baseline.mean[static_cast<std::size_t>(node)];
      const double sigma = baseline.sigma[static_cast<std::size_t>(node)];
      const double normalized = (target - mean) / sigma;
      sum += 0.5 * normalized * normalized + std::log(sigma) + 0.5 * kLogTwoPi;
      ++count;
    }
  }
  require(count > 0, "unconditional Gaussian evaluation is empty");
  return sum / static_cast<double>(count);
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

mdn::ChannelContextMdn make_model(int64_t mixture_count,
                                  const torch::Device &device) {
  seed_torch(device);
  return mdn::ChannelContextMdn(
      kInputLength, kTargetFeatureCount, kChannelCount, 1, mixture_count,
      kHiddenWidth, kResidualDepth, torch::kFloat32, device,
      kFeatureEmbeddingDim, kChannelAdapterRank,
      std::vector<int64_t>{kRawCloseCoord}, kSigmaFloor);
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
          "a direct-readout warmup is unexpectedly active");
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

struct ArmResult {
  int64_t mixture_count{0};
  Evaluation canary_initial{};
  Evaluation canary_final{};
  Evaluation fit_initial{};
  Evaluation validation_initial{};
  Evaluation fit_final{};
  Evaluation validation_final{};
  double first_step_nll{std::numeric_limits<double>::quiet_NaN()};
  double final_step_nll{std::numeric_limits<double>::quiet_NaN()};
  int64_t skipped_steps{0};
  double direct_head_parameter_max_delta{
      std::numeric_limits<double>::quiet_NaN()};
  bool canary_pass{false};
  bool direction_gate_pass{false};
  bool rank_gate_pass{false};
  bool nll_baseline_gate_pass{false};
  bool overall_pass{false};
};

ArmResult run_arm(int64_t mixture_count, const Dataset &dataset,
                  const std::vector<int64_t> &fit_indices,
                  const std::vector<int64_t> &validation_indices,
                  const std::vector<int64_t> &canary_indices,
                  const BatchSchedule &schedule, const torch::Device &device,
                  double validation_unconditional_nll) {
  ArmResult result{};
  result.mixture_count = mixture_count;

  {
    auto canary_model = make_model(mixture_count, device);
    result.canary_initial =
        evaluate(canary_model, dataset, canary_indices, device, mixture_count);
    mdn::channel_context_mdn_train_model_t canary_train(
        canary_model, kLearningRate, nll_only_options());
    const auto canary_input = make_input(dataset, canary_indices, device);
    for (int64_t step = 0; step < kCanarySteps; ++step) {
      const auto update = canary_train.train_one_batch(canary_input);
      require(!update.skipped && update.optimizer_step_applied,
              "mechanical canary optimizer step failed");
      require_nll_only_step(update);
    }
    result.canary_final = evaluate(canary_train.model(), dataset,
                                   canary_indices, device, mixture_count);
    result.canary_pass =
        result.canary_final.finite &&
        result.canary_final.edge.margin_count > 0 &&
        result.canary_final.edge.margin_pair_count > 0 &&
        result.canary_final.edge.margin_direction() >= kDirectionGate &&
        result.canary_final.edge.margin_rank() >= kRankGate &&
        result.canary_final.mean_nll < result.canary_initial.mean_nll;
  }

  auto model = make_model(mixture_count, device);
  result.fit_initial =
      evaluate(model, dataset, fit_indices, device, mixture_count);
  result.validation_initial =
      evaluate(model, dataset, validation_indices, device, mixture_count);
  const auto direct_before = clone_direct_head_parameters(model);
  mdn::channel_context_mdn_train_model_t train(model, kLearningRate,
                                               nll_only_options());

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
    require_nll_only_step(update);
    const double nll = update.nll.to(torch::kCPU).template item<double>();
    if (step == 0) {
      result.first_step_nll = nll;
    }
    result.final_step_nll = nll;
    if ((step + 1) % 500 == 0) {
      std::cerr << "arm.k" << mixture_count << " step=" << (step + 1)
                << " nll=" << std::setprecision(10) << nll << '\n';
    }
  }

  require(result.skipped_steps == 0, "main training skipped an optimizer step");
  result.direct_head_parameter_max_delta =
      direct_head_max_delta(train.model(), direct_before);
  result.fit_final =
      evaluate(train.model(), dataset, fit_indices, device, mixture_count);
  result.validation_final = evaluate(train.model(), dataset, validation_indices,
                                     device, mixture_count);
  result.direction_gate_pass =
      result.validation_final.edge.margin_count > 0 &&
      result.validation_final.edge.margin_direction() >= kDirectionGate;
  result.rank_gate_pass =
      result.validation_final.edge.margin_pair_count > 0 &&
      result.validation_final.edge.margin_rank() >= kRankGate;
  result.nll_baseline_gate_pass =
      result.validation_final.mean_nll < validation_unconditional_nll;
  result.overall_pass = result.canary_pass && result.validation_final.finite &&
                        result.direction_gate_pass && result.rank_gate_pass &&
                        result.nll_baseline_gate_pass &&
                        result.direct_head_parameter_max_delta == 0.0;
  return result;
}

const char *bool_string(bool value) { return value ? "true" : "false"; }

void emit_edge_metric(std::ostream &out, const std::string &prefix,
                      const EdgeMetric &metric) {
  out << prefix << ".count=" << metric.count << '\n';
  out << prefix << ".mae=" << metric.mae() << '\n';
  out << prefix << ".rmse=" << metric.rmse() << '\n';
  out << prefix << ".directional_accuracy=" << metric.direction() << '\n';
  out << prefix << ".margin_count=" << metric.margin_count << '\n';
  out << prefix << ".margin_directional_accuracy=" << metric.margin_direction()
      << '\n';
  out << prefix << ".pairwise_rank_count=" << metric.pair_count << '\n';
  out << prefix << ".pairwise_rank_accuracy=" << metric.rank() << '\n';
  out << prefix << ".margin_pairwise_rank_count=" << metric.margin_pair_count
      << '\n';
  out << prefix << ".margin_pairwise_rank_accuracy=" << metric.margin_rank()
      << '\n';
  out << prefix << ".best_asset_agreement=" << metric.best() << '\n';
  out << prefix << ".correlation=" << metric.correlation() << '\n';
  out << prefix << ".prediction_population_std=" << metric.prediction_std()
      << '\n';
  out << prefix << ".target_population_std=" << metric.target_std() << '\n';
  const double target_std = metric.target_std();
  out << prefix << ".prediction_to_target_std_ratio="
      << (target_std > 0.0 ? metric.prediction_std() / target_std : 0.0)
      << '\n';
}

void emit_evaluation(std::ostream &out, const std::string &prefix,
                     const Evaluation &evaluation) {
  emit_edge_metric(out, prefix, evaluation.edge);
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
  out << prefix << ".finite=" << bool_string(evaluation.finite) << '\n';
}

void emit_arm(std::ostream &out, const ArmResult &arm) {
  const std::string prefix = "arm.k" + std::to_string(arm.mixture_count);
  emit_evaluation(out, prefix + ".canary.initial", arm.canary_initial);
  emit_evaluation(out, prefix + ".canary.final", arm.canary_final);
  emit_evaluation(out, prefix + ".initial.fit", arm.fit_initial);
  emit_evaluation(out, prefix + ".initial.validation", arm.validation_initial);
  emit_evaluation(out, prefix + ".final.fit", arm.fit_final);
  emit_evaluation(out, prefix + ".final.validation", arm.validation_final);
  out << prefix << ".first_step_nll=" << arm.first_step_nll << '\n';
  out << prefix << ".final_step_nll=" << arm.final_step_nll << '\n';
  out << prefix << ".skipped_steps=" << arm.skipped_steps << '\n';
  out << prefix << ".direct_head_parameter_max_abs_delta="
      << arm.direct_head_parameter_max_delta << '\n';
  out << prefix << ".canary_pass=" << bool_string(arm.canary_pass) << '\n';
  out << prefix
      << ".direction_gate_pass=" << bool_string(arm.direction_gate_pass)
      << '\n';
  out << prefix << ".rank_gate_pass=" << bool_string(arm.rank_gate_pass)
      << '\n';
  out << prefix
      << ".nll_baseline_gate_pass=" << bool_string(arm.nll_baseline_gate_pass)
      << '\n';
  out << prefix << ".overall_pass=" << bool_string(arm.overall_pass) << '\n';
}

void write_report(const Options &options, const Dataset &dataset,
                  const std::string &schedule_sha, const torch::Device &device,
                  const EdgeMetric &previous_fit,
                  const EdgeMetric &previous_validation,
                  const EdgeMetric &seasonal_fit,
                  const EdgeMetric &seasonal_validation,
                  double unconditional_fit_nll,
                  double unconditional_validation_nll, const ArmResult &k1,
                  const ArmResult &k3) {
  std::filesystem::create_directories(options.output.parent_path());
  std::ofstream out(options.output, std::ios::trunc);
  require(out.good(), "cannot open output report");
  out << std::setprecision(12) << std::fixed;
  out << "schema_id=" << options.schema_id << '\n';
  out << "status=complete\n";
  out << "benchmark_id=synthetic_continuous_graph_v1\n";
  out << "diagnostic_authority=diagnostic_only\n";
  out << "benchmark_acceptance_authority=false\n";
  out << "question=can_the_distributional_mdn_learn_raw_causal_1d_close_"
         "history_without_the_representation_engine\n";
  out << '\n';
  out << "provenance.policy_path_used=false\n";
  out << "provenance.representation_forward_executed=false\n";
  out << "provenance.representation_checkpoint_used=false\n";
  out << "provenance.nodelift_runtime_executed=false\n";
  out << "provenance.nodelift_uniform_gauge_close_math_reproduced=true\n";
  out << "provenance.checkpoint_written=false\n";
  out << "provenance.historical_input_used=false\n";
  out << "provenance.holdout_input_used=false\n";
  out << "provenance.benchmark_contracts_validated_and_sealed_by_runner=true\n";
  out << '\n';
  out << "data.interval=1d\n";
  out << "data.normalization=previous_close_log_return\n";
  out << "data.context_definition=30_causal_uniform_gauge_node_close_returns\n";
  out << "data.target_definition=next_uniform_gauge_node_close_return\n";
  out << "data.context_shape=[B,4,1,30]\n";
  out << "data.future_shape=[B,4,1,1]\n";
  out << "data.node_order=SYNUSD,SYNALPHA,SYNBETA,SYNGAMMA\n";
  out << "data.target_coords=3\n";
  out << "data.prefix_row_count=" << kRequiredPrefixRows << '\n';
  out << "data.cross_instrument_timestamp_alignment_passed=true\n";
  out << "data.strict_1d_cadence_passed=true\n";
  out << "data.maximum_source_row_loaded=" << kMaximumSourceRowLoaded << '\n';
  out << "data.maximum_anchor_loaded=" << kMaximumAnchorLoaded << '\n';
  out << "data.anchor_zero_excluded_for_incomplete_causal_history=true\n";
  out << "data.projection_max_abs_error=" << dataset.projection_max_abs_error
      << '\n';
  out << "data.float_projection_max_abs_error="
      << dataset.float_projection_max_abs_error << '\n';
  out << "data.alpha.prefix_sha256=" << dataset.prefix_sha256[0] << '\n';
  out << "data.beta.prefix_sha256=" << dataset.prefix_sha256[1] << '\n';
  out << "data.gamma.prefix_sha256=" << dataset.prefix_sha256[2] << '\n';
  out << "data.input_combined_sha256="
      << digest::sha256_hex(dataset.prefix_sha256[0] +
                            dataset.prefix_sha256[1] + dataset.prefix_sha256[2])
      << '\n';
  out << "data.self_test_passed=true\n";
  out << '\n';
  out << "boundary.declared_fit=[" << kDeclaredFitBegin << ',' << kFitEnd
      << ")\n";
  out << "boundary.effective_fit=[" << kEffectiveFitBegin << ',' << kFitEnd
      << ")\n";
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
  out << "execution.steps=" << kTrainingSteps << '\n';
  out << "execution.batch_size=" << kBatchSize << '\n';
  out << "execution.learning_rate=" << kLearningRate << '\n';
  out << "execution.grad_clip_norm=" << kGradClipNorm << '\n';
  out << "execution.hidden_width=" << kHiddenWidth << '\n';
  out << "execution.residual_depth=" << kResidualDepth << '\n';
  out << "execution.feature_embedding_dim=" << kFeatureEmbeddingDim << '\n';
  out << "execution.channel_adapter_rank=" << kChannelAdapterRank << '\n';
  out << "execution.sigma_floor=" << kSigmaFloor << '\n';
  out << "execution.mdn_backbone_executed=true\n";
  out << "execution.mdn_distribution_executed=true\n";
  out << "execution.mdn_nll_optimized=true\n";
  out << "execution.direct_output_computed=true\n";
  out << "execution.direct_objective_enabled=false\n";
  out << "execution.direct_output_used_for_metrics=false\n";
  out << "execution.nll_only_objective_asserted_each_step=true\n";
  out << "execution.schedule_sha256=" << schedule_sha << '\n';
  out << "execution.k1_k3_schedule_identical=true\n";
  out << "gate.direction_metric=margin_directional_accuracy\n";
  out << "gate.rank_metric=margin_pairwise_rank_accuracy\n";
  out << "gate.direction_threshold=" << kDirectionGate << '\n';
  out << "gate.rank_threshold=" << kRankGate << '\n';
  out << '\n';
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
  out << '\n';
  emit_arm(out, k1);
  out << '\n';
  emit_arm(out, k3);
  out << '\n';
  out << "classification.raw_history_k1_pass=" << bool_string(k1.overall_pass)
      << '\n';
  out << "classification.raw_history_k3_pass=" << bool_string(k3.overall_pass)
      << '\n';
  std::string classification;
  if (k1.overall_pass && k3.overall_pass) {
    classification =
        "observed_both_raw_history_arms_clear_predeclared_development_gates";
  } else if (k1.overall_pass && !k3.overall_pass) {
    classification = "observed_k1_pass_k3_fail_mixture_dynamics_hypothesis";
  } else if (!k1.overall_pass && k3.overall_pass) {
    classification =
        "observed_k1_fail_k3_pass_single_gaussian_constraint_hypothesis";
  } else {
    classification =
        "observed_both_raw_history_arms_miss_predeclared_development_gates";
  }
  out << "classification.result=" << classification << '\n';
  out << "classification.causal_attribution="
         "not_established_by_single_seed_diagnostic\n";
  out << "summary=" << classification
      << "_on_raw_1d_close_history_without_representation_or_policy\n";
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
    const auto dataset = build_dataset(options);
    const auto fit_indices = range_indices(kEffectiveFitBegin, kFitEnd);
    const auto validation_indices = range_indices(kPurgeEnd, kValidationEnd);
    const auto canary_indices = range_indices(kCanaryBegin, kCanaryEnd);
    const auto schedule = make_schedule(fit_indices);

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
            "authored seasonal-lag data control failed");

    const auto baseline = fit_unconditional_gaussian(dataset, fit_indices);
    const double unconditional_fit_nll =
        unconditional_gaussian_nll(dataset, fit_indices, baseline);
    const double unconditional_validation_nll =
        unconditional_gaussian_nll(dataset, validation_indices, baseline);

    std::cerr << "running raw-history K=1 isolation on "
              << (device.is_cuda() ? "cuda" : "cpu") << '\n';
    const auto k1 =
        run_arm(1, dataset, fit_indices, validation_indices, canary_indices,
                schedule, device, unconditional_validation_nll);
    std::cerr << "running raw-history K=3 isolation on "
              << (device.is_cuda() ? "cuda" : "cpu") << '\n';
    const auto k3 =
        run_arm(3, dataset, fit_indices, validation_indices, canary_indices,
                schedule, device, unconditional_validation_nll);

    write_report(options, dataset, schedule_digest(schedule), device,
                 previous_fit, previous_validation, seasonal_fit,
                 seasonal_validation, unconditional_fit_nll,
                 unconditional_validation_nll, k1, k3);
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "error: " << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << '\n';
  }
  return 1;
}
