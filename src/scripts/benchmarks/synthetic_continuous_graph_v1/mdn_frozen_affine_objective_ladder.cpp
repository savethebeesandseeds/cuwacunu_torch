#include <algorithm>
#include <bit>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <locale>
#include <map>
#include <mutex>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include <ATen/ops/cholesky_solve.h>
#include <ATen/Context.h>
#include <ATen/Parallel.h>
#include <ATen/ops/linalg_cholesky_ex.h>
#include <ATen/ops/linalg_eigh.h>
#include <torch/data/samplers/random.h>
#include <torch/optim/lbfgs.h>
#include <torch/torch.h>
#include <torch/version.h>
#include <cuda_runtime_api.h>

#include "piaabo/digest/sha256.h"
#include "wikimyei/inference/expected_value/mdn/per_edge_affine_return_head.h"

namespace {

namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;

constexpr int64_t kFeatureDim = 128;
constexpr int64_t kDynamicFeatureCount = 3 * kFeatureDim;
constexpr int64_t kSourceFeatureCount = 400;
constexpr int64_t kEdgeCount = 3;
constexpr int64_t kChannelCount = 3;
constexpr int64_t kDevelopmentEnd = 730;
constexpr int64_t kSelectionFitEnd = 554;
constexpr int64_t kPurgeEnd = 584;
constexpr int64_t kHistoricalBegin = 760;
constexpr int64_t kHistoricalEnd = 1088;
constexpr int64_t kSealedHoldoutBegin = 1088;
constexpr int64_t kSteps = 3500;
constexpr int64_t kBatchSize = 64;
constexpr int64_t kSeed = 31;
constexpr std::uint64_t kScheduleSeed = 2031;
constexpr double kLearningRate = 1.0e-3;
constexpr double kClipNorm = 5.0;
constexpr double kDirectionRankTolerance = 0.01;
constexpr double kRmseTolerance = 5.0e-4;
constexpr double kPcaWhiteningTolerance = 5.0e-2;
constexpr double kExpectedOracleDirection = 0.805936073059;
constexpr double kExpectedOracleRank = 0.793759512938;
constexpr double kExpectedPinnedOneThreadOracleRmse = 0.020836577214;
constexpr double kLegacyCanonicalOracleRmse = 0.020836575958;
constexpr double kLegacyOracleRmseDriftBound = 2.0e-9;
constexpr std::string_view kFitProbeSchema =
    "synthetic_mdn_frozen_feature_capture.v1";
constexpr std::string_view kProbeRecordSchema =
    "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1";
constexpr std::string_view kExpectedGraphFingerprint = "d334e38b1887ae16";
constexpr std::string_view kExpectedSidecarSemanticDigest =
    "b0f3f00760d92d3026ed8675c9d6f572dd1c0b5de6f1c45bbd8b9253f99fd709";
constexpr std::string_view kExpectedResultSchema =
    "synthetic_mdn_frozen_affine_objective_ladder_v1";

constexpr std::string_view kProbeHeader =
    "record_schema,anchor_key,anchor_index,anchor_local_index,edge_index,"
    "edge_id,base_node_id,quote_node_id,channel_index,"
    "target_edge_close_return,feature_count,feature_values";

struct Options {
  std::filesystem::path input;
  std::filesystem::path evaluation_input;
  std::filesystem::path oracle_archive;
  std::filesystem::path output;
  std::string schema_id;
};

struct Dataset {
  torch::Tensor context;          // [A,E+1,C,H], CPU float32.
  torch::Tensor readout_features; // [A,E,C,400], CPU float32.
  torch::Tensor dynamic_features; // [A,E,C,384], CPU float32.
  torch::Tensor target;           // [A,E,C], CPU float32.
  std::vector<int64_t> anchors;
  std::vector<std::string> edge_ids;
  std::vector<std::string> base_node_ids;
  std::string quote_node_id;
  std::string record_schema;
  int64_t row_count{0};
  double prefix_max_abs_delta{std::numeric_limits<double>::infinity()};
  bool prefix_torch_equal{false};
};

struct Metric {
  int64_t count{0};
  int64_t direction_correct{0};
  int64_t pair_count{0};
  int64_t pair_correct{0};
  double abs_error{0.0};
  double sq_error{0.0};
  double prediction_sum{0.0};
  double prediction_sq_sum{0.0};
  double target_sum{0.0};
  double target_sq_sum{0.0};
  double cross_sum{0.0};
};

enum class ObjectiveKind { RawMse, ScaledHuber, Direction, Rank };

struct ObjectiveParts {
  torch::Tensor total;
  torch::Tensor raw_mse;
  torch::Tensor scaled_huber;
  torch::Tensor direction;
  torch::Tensor rank;
};

struct Normalization {
  torch::Tensor mean;    // [F], CPU float32.
  torch::Tensor inv_std; // [F], CPU float32.
};

struct ModelState {
  torch::Tensor mean;
  torch::Tensor inv_std;
  torch::Tensor weights;
  torch::Tensor bias;
};

struct PcaSpec {
  torch::Tensor center;    // [E,F], CPU float32.
  torch::Tensor transform; // [E,F,K], CPU float32.
  std::vector<int64_t> ranks;
  std::vector<double> diagonal_max_abs_error;
  std::vector<double> off_diagonal_max_abs;
};

std::vector<std::string> split_exact(const std::string &text,
                                     char delimiter) {
  std::vector<std::string> values;
  std::size_t begin = 0;
  for (;;) {
    const auto end = text.find(delimiter, begin);
    values.push_back(text.substr(begin, end - begin));
    if (end == std::string::npos) {
      break;
    }
    begin = end + 1;
  }
  return values;
}

int64_t parse_i64_strict(const std::string &text, const char *name) {
  if (text.empty()) {
    throw std::runtime_error(std::string("empty integer field: ") + name);
  }
  std::size_t consumed = 0;
  const auto value = std::stoll(text, &consumed, 10);
  if (consumed != text.size()) {
    throw std::runtime_error(std::string("invalid integer field: ") + name);
  }
  return value;
}

double parse_f64_strict(const std::string &text, const char *name) {
  if (text.empty()) {
    throw std::runtime_error(std::string("empty floating field: ") + name);
  }
  std::size_t consumed = 0;
  const auto value = std::stod(text, &consumed);
  if (consumed != text.size() || !std::isfinite(value)) {
    throw std::runtime_error(std::string("invalid floating field: ") + name);
  }
  return value;
}

std::filesystem::path normalized_path(const std::filesystem::path &path) {
  std::error_code error;
  auto absolute = std::filesystem::absolute(path, error);
  if (error) {
    throw std::runtime_error("failed to normalize a CLI path");
  }
  return absolute.lexically_normal();
}

Options parse_options(int argc, char **argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    auto value = [&](const char *name) -> std::string {
      if (index + 1 >= argc) {
        throw std::runtime_error(std::string("missing value for ") + name);
      }
      return argv[++index];
    };
    if (argument == "--input") {
      options.input = value("--input");
    } else if (argument == "--evaluation-input") {
      options.evaluation_input = value("--evaluation-input");
    } else if (argument == "--oracle-archive") {
      options.oracle_archive = value("--oracle-archive");
    } else if (argument == "--output") {
      options.output = value("--output");
    } else if (argument == "--schema-id") {
      options.schema_id = value("--schema-id");
    } else {
      throw std::runtime_error("unknown argument: " + argument);
    }
  }
  if (options.input.empty() || options.evaluation_input.empty() ||
      options.oracle_archive.empty() || options.output.empty() ||
      options.schema_id.empty()) {
    throw std::runtime_error(
        "--input --evaluation-input --oracle-archive --output and "
        "--schema-id are required");
  }
  const std::set<std::filesystem::path> inputs{
      normalized_path(options.input), normalized_path(options.evaluation_input),
      normalized_path(options.oracle_archive)};
  if (inputs.size() != 3 || inputs.contains(normalized_path(options.output))) {
    throw std::runtime_error("all inputs and the output must be distinct");
  }
  return options;
}

std::string sha256_file(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("failed to open an input for SHA-256");
  }
  const std::string bytes((std::istreambuf_iterator<char>(input)),
                          std::istreambuf_iterator<char>());
  if (!input.good() && !input.eof()) {
    throw std::runtime_error("failed while hashing an input");
  }
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

Dataset read_probe(const std::filesystem::path &path, int64_t expected_begin,
                   int64_t expected_end) {
  if (expected_begin < 0 || expected_end <= expected_begin ||
      expected_end > kSealedHoldoutBegin) {
    throw std::runtime_error("invalid predeclared probe range");
  }
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open a probe input");
  }
  std::string line;
  if (!std::getline(input, line) || line != kProbeHeader) {
    throw std::runtime_error("probe header does not match the frozen schema");
  }

  const int64_t anchor_count = expected_end - expected_begin;
  const auto nan = std::numeric_limits<float>::quiet_NaN();
  std::vector<float> readout(static_cast<std::size_t>(
                                 anchor_count * kEdgeCount * kChannelCount *
                                 kSourceFeatureCount),
                             nan);
  std::vector<float> target(
      static_cast<std::size_t>(anchor_count * kEdgeCount * kChannelCount), nan);
  std::vector<bool> seen(
      static_cast<std::size_t>(anchor_count * kEdgeCount * kChannelCount),
      false);
  std::vector<std::string> edge_ids(static_cast<std::size_t>(kEdgeCount));
  std::vector<std::string> base_ids(static_cast<std::size_t>(kEdgeCount));
  std::string quote_id;
  std::string record_schema;
  int64_t row_count = 0;

  const auto coordinate = [](int64_t a, int64_t e, int64_t c) {
    return static_cast<std::size_t>((a * kEdgeCount + e) * kChannelCount + c);
  };
  const auto feature_coordinate = [](int64_t a, int64_t e, int64_t c,
                                     int64_t f) {
    return static_cast<std::size_t>(
        (((a * kEdgeCount + e) * kChannelCount + c) * kSourceFeatureCount) +
        f);
  };

  while (std::getline(input, line)) {
    if (line.empty()) {
      throw std::runtime_error("probe contains an empty data row");
    }
    const auto columns = split_exact(line, ',');
    if (columns.size() != 12) {
      throw std::runtime_error("probe row must contain exactly 12 columns");
    }
    if (record_schema.empty()) {
      record_schema = columns[0];
    } else if (record_schema != columns[0]) {
      throw std::runtime_error("probe record schema changes within the file");
    }
    (void)parse_i64_strict(columns[1], "anchor_key");
    const auto anchor = parse_i64_strict(columns[2], "anchor_index");
    (void)parse_i64_strict(columns[3], "anchor_local_index");
    const auto edge = parse_i64_strict(columns[4], "edge_index");
    const auto channel = parse_i64_strict(columns[8], "channel_index");
    const auto target_value = parse_f64_strict(columns[9], "target");
    const auto feature_count =
        parse_i64_strict(columns[10], "feature_count");
    if (anchor >= kSealedHoldoutBegin || anchor < expected_begin ||
        anchor >= expected_end || edge < 0 || edge >= kEdgeCount ||
        channel < 0 || channel >= kChannelCount ||
        feature_count != kSourceFeatureCount || columns[5].empty() ||
        columns[6].empty() || columns[7].empty()) {
      throw std::runtime_error("probe row violates its predeclared domain");
    }
    const auto feature_cells = split_exact(columns[11], ';');
    if (static_cast<int64_t>(feature_cells.size()) != kSourceFeatureCount) {
      throw std::runtime_error("probe feature width is not exactly 400");
    }
    const int64_t local_anchor = anchor - expected_begin;
    const auto position = coordinate(local_anchor, edge, channel);
    if (seen[position]) {
      throw std::runtime_error("duplicate anchor/edge/channel probe row");
    }
    seen[position] = true;
    target[position] = static_cast<float>(target_value);
    for (int64_t feature = 0; feature < kSourceFeatureCount; ++feature) {
      readout[feature_coordinate(local_anchor, edge, channel, feature)] =
          static_cast<float>(parse_f64_strict(
              feature_cells[static_cast<std::size_t>(feature)], "feature"));
    }
    const auto edge_slot = static_cast<std::size_t>(edge);
    if (edge_ids[edge_slot].empty()) {
      edge_ids[edge_slot] = columns[5];
      base_ids[edge_slot] = columns[6];
    } else if (edge_ids[edge_slot] != columns[5] ||
               base_ids[edge_slot] != columns[6]) {
      throw std::runtime_error("edge identity changes within the probe");
    }
    if (quote_id.empty()) {
      quote_id = columns[7];
    } else if (quote_id != columns[7]) {
      throw std::runtime_error("quote identity changes within the probe");
    }
    ++row_count;
  }
  if (row_count != anchor_count * kEdgeCount * kChannelCount ||
      std::find(seen.begin(), seen.end(), false) != seen.end()) {
    throw std::runtime_error("probe does not contain complete anchor groups");
  }
  if (std::set<std::string>(edge_ids.begin(), edge_ids.end()).size() !=
          static_cast<std::size_t>(kEdgeCount) ||
      std::set<std::string>(base_ids.begin(), base_ids.end()).size() !=
          static_cast<std::size_t>(kEdgeCount)) {
    throw std::runtime_error("probe identities are incomplete or duplicated");
  }

  Dataset dataset;
  dataset.readout_features =
      torch::from_blob(readout.data(),
                       {anchor_count, kEdgeCount, kChannelCount,
                        kSourceFeatureCount},
                       torch::kFloat32)
          .clone();
  dataset.target =
      torch::from_blob(target.data(),
                       {anchor_count, kEdgeCount, kChannelCount},
                       torch::kFloat32)
          .clone();
  auto quote = dataset.readout_features.select(1, 0)
                   .narrow(2, kFeatureDim, kFeatureDim)
                   .clone();
  for (int64_t edge = 1; edge < kEdgeCount; ++edge) {
    const auto edge_quote = dataset.readout_features.select(1, edge)
                                .narrow(2, kFeatureDim, kFeatureDim);
    if (!torch::equal(quote, edge_quote)) {
      throw std::runtime_error(
          "quote features are not bit-identical across edge rows");
    }
  }
  std::vector<torch::Tensor> nodes{quote.unsqueeze(1)};
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    nodes.push_back(dataset.readout_features.select(1, edge)
                        .narrow(2, 0, kFeatureDim)
                        .unsqueeze(1));
  }
  dataset.context = torch::cat(nodes, 1).contiguous();
  auto expanded_quote = dataset.context.select(1, 0)
                            .unsqueeze(1)
                            .expand({anchor_count, kEdgeCount, kChannelCount,
                                     kFeatureDim});
  auto bases = dataset.context.narrow(1, 1, kEdgeCount);
  auto reconstructed =
      torch::cat({bases, expanded_quote, bases - expanded_quote}, -1)
          .contiguous();
  dataset.dynamic_features =
      dataset.readout_features.narrow(3, 0, kDynamicFeatureCount).contiguous();
  dataset.prefix_torch_equal =
      torch::equal(reconstructed, dataset.dynamic_features);
  dataset.prefix_max_abs_delta =
      (reconstructed - dataset.dynamic_features).abs().max().item<double>();
  if (!dataset.prefix_torch_equal || dataset.prefix_max_abs_delta != 0.0) {
    throw std::runtime_error(
        "cached readout prefix is not the exact reconstructed context");
  }
  dataset.anchors.resize(static_cast<std::size_t>(anchor_count));
  std::iota(dataset.anchors.begin(), dataset.anchors.end(), expected_begin);
  dataset.edge_ids = std::move(edge_ids);
  dataset.base_node_ids = std::move(base_ids);
  dataset.quote_node_id = std::move(quote_id);
  dataset.record_schema = std::move(record_schema);
  dataset.row_count = row_count;
  return dataset;
}

std::vector<int64_t> integer_range(int64_t begin, int64_t end) {
  if (begin < 0 || end < begin) {
    throw std::runtime_error("invalid integer range");
  }
  std::vector<int64_t> result(static_cast<std::size_t>(end - begin));
  std::iota(result.begin(), result.end(), begin);
  return result;
}

torch::Tensor index_tensor(const std::vector<int64_t> &indices,
                           const torch::Device &device) {
  return torch::tensor(indices, torch::TensorOptions().dtype(torch::kInt64))
      .to(device);
}

Metric evaluate_prediction(const torch::Tensor &prediction_input,
                           const torch::Tensor &target_input) {
  const auto prediction =
      prediction_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  const auto target =
      target_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  if (prediction.sizes() != target.sizes() || prediction.dim() != 3) {
    throw std::runtime_error("metric tensor shape mismatch");
  }
  Metric metric;
  const auto p = prediction.accessor<float, 3>();
  const auto t = target.accessor<float, 3>();
  const auto sign = [](double value) {
    return value > 0.0 ? 1 : (value < 0.0 ? -1 : 0);
  };
  for (int64_t anchor = 0; anchor < prediction.size(0); ++anchor) {
    for (int64_t channel = 0; channel < prediction.size(2); ++channel) {
      for (int64_t edge = 0; edge < prediction.size(1); ++edge) {
        const double predicted = p[anchor][edge][channel];
        const double realized = t[anchor][edge][channel];
        ++metric.count;
        metric.abs_error += std::fabs(predicted - realized);
        metric.sq_error += (predicted - realized) * (predicted - realized);
        metric.prediction_sum += predicted;
        metric.prediction_sq_sum += predicted * predicted;
        metric.target_sum += realized;
        metric.target_sq_sum += realized * realized;
        metric.cross_sum += predicted * realized;
        if (sign(predicted) == sign(realized)) {
          ++metric.direction_correct;
        }
      }
      for (int64_t lhs = 0; lhs < prediction.size(1); ++lhs) {
        for (int64_t rhs = lhs + 1; rhs < prediction.size(1); ++rhs) {
          ++metric.pair_count;
          if (sign(p[anchor][lhs][channel] - p[anchor][rhs][channel]) ==
              sign(t[anchor][lhs][channel] - t[anchor][rhs][channel])) {
            ++metric.pair_correct;
          }
        }
      }
    }
  }
  return metric;
}

double metric_rmse(const Metric &metric) {
  return metric.count > 0
             ? std::sqrt(metric.sq_error / static_cast<double>(metric.count))
             : std::numeric_limits<double>::infinity();
}

double metric_direction(const Metric &metric) {
  return metric.count > 0
             ? static_cast<double>(metric.direction_correct) /
                   static_cast<double>(metric.count)
             : 0.0;
}

double metric_rank(const Metric &metric) {
  return metric.pair_count > 0
             ? static_cast<double>(metric.pair_correct) /
                   static_cast<double>(metric.pair_count)
             : 0.0;
}

double metric_correlation(const Metric &metric) {
  if (metric.count <= 1) {
    return 0.0;
  }
  const auto n = static_cast<double>(metric.count);
  const auto numerator =
      metric.cross_sum - metric.prediction_sum * metric.target_sum / n;
  const auto lhs = metric.prediction_sq_sum -
                   metric.prediction_sum * metric.prediction_sum / n;
  const auto rhs =
      metric.target_sq_sum - metric.target_sum * metric.target_sum / n;
  return lhs > 0.0 && rhs > 0.0 ? numerator / std::sqrt(lhs * rhs) : 0.0;
}

void require_metric_counts(const Metric &metric, int64_t expected_count,
                           int64_t expected_pair_count,
                           const char *surface) {
  if (metric.count != expected_count ||
      metric.pair_count != expected_pair_count) {
    throw std::runtime_error(std::string("unexpected metric domain: ") +
                             surface);
  }
}

ObjectiveParts compute_objective(const torch::Tensor &prediction,
                                 const torch::Tensor &target,
                                 ObjectiveKind kind) {
  const auto raw_mse = (prediction - target).pow(2).mean();
  const auto scaled_prediction = 36.0 * prediction;
  const auto scaled_target = 36.0 * target;
  const auto scaled_difference = scaled_prediction - scaled_target;
  const auto absolute_difference = scaled_difference.abs();
  const auto scaled_huber =
      torch::where(absolute_difference < 0.5, scaled_difference.pow(2),
                   absolute_difference - 0.25)
          .mean();
  const auto target_sign = target.sign();
  const auto active_direction = target_sign.ne(0);
  auto direction = prediction.new_zeros({});
  if (active_direction.any().item<bool>()) {
    direction = torch::softplus(
                    -(scaled_prediction.masked_select(active_direction) *
                      target_sign.masked_select(active_direction)))
                    .mean();
  }
  auto rank_sum = prediction.new_zeros({});
  int64_t rank_count = 0;
  for (int64_t lhs = 0; lhs < prediction.size(1); ++lhs) {
    for (int64_t rhs = lhs + 1; rhs < prediction.size(1); ++rhs) {
      const auto predicted_difference =
          36.0 * (prediction.select(1, lhs) - prediction.select(1, rhs));
      const auto rank_sign =
          (target.select(1, lhs) - target.select(1, rhs)).sign();
      const auto active = rank_sign.ne(0);
      const auto count = active.sum().item<int64_t>();
      if (count > 0) {
        rank_sum =
            rank_sum +
            torch::softplus(-(predicted_difference.masked_select(active) *
                              rank_sign.masked_select(active)))
                .sum();
        rank_count += count;
      }
    }
  }
  const auto rank = rank_count > 0
                        ? rank_sum / static_cast<double>(rank_count)
                        : prediction.new_zeros({});
  torch::Tensor total;
  switch (kind) {
  case ObjectiveKind::RawMse:
    total = raw_mse;
    break;
  case ObjectiveKind::ScaledHuber:
    total = 100.0 * scaled_huber;
    break;
  case ObjectiveKind::Direction:
    total = 100.0 * scaled_huber + 5.0 * direction;
    break;
  case ObjectiveKind::Rank:
    total = 100.0 * scaled_huber + 5.0 * direction + 5.0 * rank;
    break;
  }
  return {total, raw_mse, scaled_huber, direction, rank};
}

Normalization compute_normalization(const torch::Tensor &features,
                                    const std::vector<int64_t> &indices) {
  const auto selected =
      features.index_select(0, index_tensor(indices, torch::kCPU));
  const auto flat = selected.reshape({-1, kDynamicFeatureCount});
  const auto mean = flat.mean(0);
  const auto variance = (flat - mean).pow(2).mean(0);
  const auto standard_deviation = variance.sqrt();
  const auto inv_std = torch::where(standard_deviation > 1.0e-12,
                                    standard_deviation.reciprocal(),
                                    torch::ones_like(standard_deviation));
  return {mean.contiguous(), inv_std.contiguous()};
}

struct TrainableAffineImpl : torch::nn::Module {
  bool uses_pca{false};
  int64_t reduced_dim{kDynamicFeatureCount};
  torch::Tensor feature_mean;
  torch::Tensor feature_inv_std;
  torch::Tensor pca_center;
  torch::Tensor pca_transform;
  torch::Tensor weights;
  torch::Tensor bias;

  TrainableAffineImpl(const Normalization &normalization,
                      const std::optional<PcaSpec> &pca) {
    uses_pca = pca.has_value();
    if (uses_pca) {
      reduced_dim = pca->transform.size(2);
    }
    feature_mean = register_buffer(
        "feature_mean", normalization.mean.view({1, 1, 1, -1}).clone());
    feature_inv_std = register_buffer(
        "feature_inv_std",
        normalization.inv_std.view({1, 1, 1, -1}).clone());
    if (uses_pca) {
      pca_center = register_buffer(
          "pca_center", pca->center.view({1, kEdgeCount, 1, -1}).clone());
      pca_transform =
          register_buffer("pca_transform", pca->transform.clone());
    }
    weights = register_parameter(
        "weights", torch::zeros({kEdgeCount, reduced_dim}, torch::kFloat32));
    bias = register_parameter("bias",
                              torch::zeros({kEdgeCount}, torch::kFloat32));
  }

  torch::Tensor forward(const torch::Tensor &dynamic_features) {
    auto standardized =
        (dynamic_features - feature_mean) * feature_inv_std;
    if (!uses_pca) {
      return (standardized * weights.view({1, kEdgeCount, 1, reduced_dim}))
                 .sum(-1) +
             bias.view({1, kEdgeCount, 1});
    }
    standardized = standardized - pca_center;
    std::vector<torch::Tensor> predictions;
    predictions.reserve(static_cast<std::size_t>(kEdgeCount));
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      const auto reduced = standardized.select(1, edge).matmul(
          pca_transform.select(0, edge));
      predictions.push_back(
          (reduced * weights.select(0, edge).view({1, 1, reduced_dim}))
                  .sum(-1) +
              bias.select(0, edge));
    }
    return torch::stack(predictions, 1);
  }

  ModelState mapped_state() const {
    torch::NoGradGuard no_grad;
    auto ordinary_weights = weights;
    auto ordinary_bias = bias;
    if (uses_pca) {
      std::vector<torch::Tensor> rows;
      std::vector<torch::Tensor> biases;
      rows.reserve(static_cast<std::size_t>(kEdgeCount));
      biases.reserve(static_cast<std::size_t>(kEdgeCount));
      for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
        const auto row = pca_transform.select(0, edge).matmul(
            weights.select(0, edge).unsqueeze(1)).squeeze(1);
        rows.push_back(row);
        biases.push_back(bias.select(0, edge) -
                         pca_center.reshape({kEdgeCount, kDynamicFeatureCount})
                             .select(0, edge)
                             .dot(row));
      }
      ordinary_weights = torch::stack(rows, 0);
      ordinary_bias = torch::stack(biases, 0);
    }
    return {.mean = feature_mean.reshape({-1}).detach().to(torch::kCPU).clone(),
            .inv_std =
                feature_inv_std.reshape({-1}).detach().to(torch::kCPU).clone(),
            .weights = ordinary_weights.detach().to(torch::kCPU).clone(),
            .bias = ordinary_bias.detach().to(torch::kCPU).clone()};
  }
};

using TrainableAffine = std::shared_ptr<TrainableAffineImpl>;

void append_u32_be(std::string &bytes, std::uint32_t value) {
  for (int shift = 24; shift >= 0; shift -= 8) {
    bytes.push_back(static_cast<char>((value >> shift) & 0xffU));
  }
}

void append_u64_be(std::string &bytes, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    bytes.push_back(static_cast<char>((value >> shift) & 0xffU));
  }
}

void append_float32_tensor(std::string &bytes, const std::string &name,
                           const torch::Tensor &input) {
  const auto tensor =
      input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  if (!torch::isfinite(tensor).all().item<bool>()) {
    throw std::runtime_error("cannot digest a non-finite tensor");
  }
  bytes += "tensor=" + name + "\nrank=" + std::to_string(tensor.dim()) +
           "\n";
  for (int64_t dimension = 0; dimension < tensor.dim(); ++dimension) {
    bytes += "shape." + std::to_string(dimension) + "=" +
             std::to_string(tensor.size(dimension)) + "\n";
  }
  const auto *values = tensor.data_ptr<float>();
  for (int64_t index = 0; index < tensor.numel(); ++index) {
    append_u32_be(bytes, std::bit_cast<std::uint32_t>(values[index]));
  }
}

std::string prediction_digest(const torch::Tensor &prediction) {
  std::string bytes =
      "synthetic_mdn_frozen_affine_objective_ladder_prediction.v1\n";
  append_float32_tensor(bytes, "prediction", prediction);
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

std::string state_digest(const ModelState &state) {
  std::string bytes =
      "synthetic_mdn_frozen_affine_objective_ladder_state.v1\n";
  append_float32_tensor(bytes, "feature_mean", state.mean);
  append_float32_tensor(bytes, "feature_inv_std", state.inv_std);
  append_float32_tensor(bytes, "weights", state.weights);
  append_float32_tensor(bytes, "bias", state.bias);
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

struct AnalyticFit {
  ModelState state; // CPU float64.
  double maximum_normalized_residual{0.0};
  double coefficient_l2_norm{0.0};
};

torch::Tensor context_derived_features_float64(
    const torch::Tensor &context_input) {
  const auto context =
      context_input.to(torch::kCPU, torch::kFloat64).contiguous();
  if (context.dim() != 4 || context.size(1) != kEdgeCount + 1 ||
      context.size(2) != kChannelCount || context.size(3) != kFeatureDim) {
    throw std::runtime_error("analytic context surface shape mismatch");
  }
  const auto anchor_count = context.size(0);
  const auto quote = context.select(1, 0)
                         .unsqueeze(1)
                         .expand({anchor_count, kEdgeCount, kChannelCount,
                                  kFeatureDim});
  const auto bases = context.narrow(1, 1, kEdgeCount);
  return torch::cat({bases, quote, bases - quote}, -1).contiguous();
}

AnalyticFit fit_analytic_ridge(const Dataset &dataset,
                               const std::vector<int64_t> &fit_indices,
                               double alpha) {
  if (fit_indices.empty() || !(alpha > 0.0)) {
    throw std::runtime_error("analytic ridge requires a positive fit domain");
  }
  torch::NoGradGuard no_grad;
  const auto selected = index_tensor(fit_indices, torch::kCPU);
  const auto fit_context = dataset.context.index_select(0, selected);
  const auto features = context_derived_features_float64(fit_context);
  const auto target =
      dataset.target.index_select(0, selected).to(torch::kFloat64);
  const auto anchor_count = features.size(0);
  const auto flat = features.reshape({-1, kDynamicFeatureCount});
  const auto mean = flat.mean(0);
  const auto variance = (flat - mean).pow(2).mean(0);
  const auto standard_deviation = variance.sqrt();
  const auto inv_std = torch::where(standard_deviation > 1.0e-12,
                                    standard_deviation.reciprocal(),
                                    torch::ones_like(standard_deviation));
  const auto standardized =
      (features - mean.view({1, 1, 1, kDynamicFeatureCount})) *
      inv_std.view({1, 1, 1, kDynamicFeatureCount});
  auto weights =
      torch::zeros({kEdgeCount, kDynamicFeatureCount}, torch::kFloat64);
  auto bias = torch::zeros({kEdgeCount}, torch::kFloat64);
  double maximum_residual = 0.0;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto x = standardized.select(1, edge).contiguous().reshape(
        {anchor_count * kChannelCount, kDynamicFeatureCount});
    const auto y = target.select(1, edge).contiguous().reshape({-1});
    const auto x_mean = x.mean(0);
    const auto y_mean = y.mean();
    const auto centered_x = x - x_mean;
    const auto centered_y = y - y_mean;
    auto gram = centered_x.transpose(0, 1).matmul(centered_x);
    gram.diagonal(0, 0, 1).add_(static_cast<double>(x.size(0)) * alpha);
    const auto rhs = centered_x.transpose(0, 1).matmul(centered_y.unsqueeze(1));
    auto [cholesky, info] =
        at::linalg_cholesky_ex(gram, false, false);
    if (info.item<int64_t>() != 0) {
      throw std::runtime_error("selection oracle Cholesky failed");
    }
    const auto row = at::cholesky_solve(rhs, cholesky, false).squeeze(1);
    const auto residual = gram.matmul(row.unsqueeze(1)) - rhs;
    const auto normalized_residual =
        residual.norm().item<double>() /
        std::max(rhs.norm().item<double>(), 1.0e-30);
    if (!std::isfinite(normalized_residual) || normalized_residual > 1.0e-7 ||
        !torch::isfinite(row).all().item<bool>()) {
      throw std::runtime_error("selection oracle solve is numerically invalid");
    }
    maximum_residual = std::max(maximum_residual, normalized_residual);
    weights.select(0, edge).copy_(row);
    bias.select(0, edge).copy_(y_mean - x_mean.dot(row));
  }
  return {{mean.contiguous(), inv_std.contiguous(), weights.contiguous(),
           bias.contiguous()},
          maximum_residual, weights.norm().item<double>()};
}

torch::Tensor predict_state(const ModelState &state,
                            const torch::Tensor &dynamic_features) {
  torch::NoGradGuard no_grad;
  const auto output_type = dynamic_features.scalar_type();
  const auto features = dynamic_features.to(torch::kCPU, torch::kFloat64);
  const auto standardized =
      (features - state.mean.to(torch::kFloat64).view({1, 1, 1, -1})) *
      state.inv_std.to(torch::kFloat64).view({1, 1, 1, -1});
  return ((standardized *
           state.weights.to(torch::kFloat64).view(
               {1, kEdgeCount, 1, kDynamicFeatureCount}))
              .sum(-1) +
          state.bias.to(torch::kFloat64).view({1, kEdgeCount, 1}))
      .to(output_type);
}

torch::Tensor predict_state_from_context(const ModelState &state,
                                         const torch::Tensor &context) {
  const auto prediction =
      predict_state(state, context_derived_features_float64(context));
  return prediction.to(context.scalar_type());
}

void validate_sidecar_binding(
    const mdn::LoadedPerEdgeAffineReturnHead &loaded, const Dataset &development,
    const std::string &development_sha256) {
  const auto &metadata = loaded.metadata;
  if (metadata.feature_dim != kFeatureDim ||
      metadata.edge_count != kEdgeCount ||
      metadata.readout_feature_dim != kSourceFeatureCount ||
      metadata.channel_count != kChannelCount || metadata.quote_node_index != 0 ||
      metadata.selection_train_begin != 0 ||
      metadata.selection_train_end != kSelectionFitEnd ||
      metadata.validation_purge_begin != kSelectionFitEnd ||
      metadata.validation_purge_end != kPurgeEnd ||
      metadata.validation_begin != kPurgeEnd ||
      metadata.validation_end != kDevelopmentEnd || metadata.refit_begin != 0 ||
      metadata.refit_end != kDevelopmentEnd ||
      metadata.valid_from_anchor != kDevelopmentEnd ||
      std::fabs(metadata.selected_alpha - 1.0e-10) > 1.0e-24 ||
      metadata.fit_probe_sha256 != development_sha256 ||
      metadata.fit_probe_schema_id != kFitProbeSchema ||
      metadata.graph_order_fingerprint != kExpectedGraphFingerprint ||
      metadata.semantic_tensor_digest != kExpectedSidecarSemanticDigest ||
      metadata.edge_ids != development.edge_ids) {
    throw std::runtime_error(
        "loaded PerEdgeAffineReturnHead is not bound to these frozen probes");
  }
  std::vector<std::string> expected_nodes{development.quote_node_id};
  expected_nodes.insert(expected_nodes.end(), development.base_node_ids.begin(),
                        development.base_node_ids.end());
  if (metadata.node_ids != expected_nodes ||
      mdn::semantic_tensor_digest(loaded.head, metadata) !=
          metadata.semantic_tensor_digest) {
    throw std::runtime_error("sidecar identity or semantic digest mismatch");
  }
}

void validate_historical_identity(const Dataset &development,
                                  const Dataset &historical) {
  if (historical.edge_ids != development.edge_ids ||
      historical.base_node_ids != development.base_node_ids ||
      historical.quote_node_id != development.quote_node_id ||
      historical.record_schema != development.record_schema) {
    throw std::runtime_error(
        "historical diagnostic identity differs from development");
  }
}

class ScopedCpuRngSeed {
public:
  explicit ScopedCpuRngSeed(std::uint64_t seed)
      : generator_(at::globalContext().defaultGenerator(c10::DeviceType::CPU)) {
    std::lock_guard<std::mutex> lock(generator_.mutex());
    saved_state_ = generator_.get_state();
    generator_.set_current_seed(seed);
  }

  ScopedCpuRngSeed(const ScopedCpuRngSeed &) = delete;
  ScopedCpuRngSeed &operator=(const ScopedCpuRngSeed &) = delete;

  ~ScopedCpuRngSeed() {
    std::lock_guard<std::mutex> lock(generator_.mutex());
    generator_.set_state(saved_state_);
  }

private:
  at::Generator generator_;
  at::Tensor saved_state_;
};

struct BatchSchedule {
  std::vector<std::vector<int64_t>> batches;
  std::vector<int64_t> visit_counts;
  std::map<int64_t, int64_t> batch_size_histogram;
  int64_t epoch_count{0};
  std::string digest;
};

BatchSchedule make_batch_schedule(int64_t anchor_count, int64_t steps) {
  if (anchor_count <= 0 || steps <= 0) {
    throw std::runtime_error("invalid schedule dimensions");
  }
  BatchSchedule schedule;
  schedule.visit_counts.assign(static_cast<std::size_t>(anchor_count), 0);
  torch::data::samplers::RandomSampler sampler(anchor_count);
  for (int64_t epoch = 0;
       static_cast<int64_t>(schedule.batches.size()) < steps; ++epoch) {
    {
      ScopedCpuRngSeed guard(kScheduleSeed +
                             static_cast<std::uint64_t>(epoch));
      sampler.reset();
    }
    ++schedule.epoch_count;
    for (;;) {
      const auto sampled = sampler.next(static_cast<std::size_t>(kBatchSize));
      if (!sampled.has_value()) {
        break;
      }
      std::vector<int64_t> batch;
      batch.reserve(sampled->size());
      for (const auto value : *sampled) {
        if (value >= static_cast<std::size_t>(anchor_count)) {
          throw std::runtime_error("sampler emitted an out-of-range anchor");
        }
        batch.push_back(static_cast<int64_t>(value));
        ++schedule.visit_counts[value];
      }
      ++schedule.batch_size_histogram[static_cast<int64_t>(batch.size())];
      schedule.batches.push_back(std::move(batch));
      if (static_cast<int64_t>(schedule.batches.size()) == steps) {
        break;
      }
    }
  }
  std::string bytes =
      "synthetic_mdn_production_epoch_reseeded_batch_schedule.v1\n";
  append_u64_be(bytes, static_cast<std::uint64_t>(anchor_count));
  append_u64_be(bytes, static_cast<std::uint64_t>(steps));
  append_u64_be(bytes, kScheduleSeed);
  for (const auto &batch : schedule.batches) {
    append_u64_be(bytes, static_cast<std::uint64_t>(batch.size()));
    for (const auto index : batch) {
      append_u64_be(bytes, static_cast<std::uint64_t>(index));
    }
  }
  schedule.digest = cuwacunu::piaabo::digest::sha256_hex(bytes);
  return schedule;
}

struct GramDiagnostic {
  std::string surface;
  int64_t edge{0};
  int64_t observations{0};
  int64_t dimension{0};
  int64_t retained_rank{0};
  int64_t nullity{0};
  double lambda_min{0.0};
  double lambda_q25{0.0};
  double lambda_median{0.0};
  double lambda_q75{0.0};
  double lambda_max{0.0};
  double retained_condition{std::numeric_limits<double>::infinity()};
  double stable_rank{0.0};
  double effective_rank{0.0};
  double correlation_max_abs_off_diagonal{0.0};
  double correlation_p99_abs_off_diagonal{0.0};
  double target_std{0.0};
  bool psd_pass{false};
};

double ordered_quantile(const torch::Tensor &ascending, double quantile) {
  const auto count = ascending.numel();
  if (count <= 0) {
    return 0.0;
  }
  const auto position = static_cast<int64_t>(
      std::llround(quantile * static_cast<double>(count - 1)));
  return ascending[position].item<double>();
}

GramDiagnostic diagnose_gram(const std::string &surface, int64_t edge,
                             const torch::Tensor &x_input,
                             const torch::Tensor &target_input) {
  torch::NoGradGuard no_grad;
  const auto x = x_input.to(torch::kCPU, torch::kFloat64).contiguous();
  const auto centered = x - x.mean(0);
  auto covariance = centered.transpose(0, 1).matmul(centered) /
                    static_cast<double>(centered.size(0));
  covariance = 0.5 * (covariance + covariance.transpose(0, 1));
  const auto eigenvalues = at::linalg_eigh(covariance, "L");
  const auto values = std::get<0>(eigenvalues).contiguous();
  const double lambda_max = values[-1].item<double>();
  const double threshold = std::max(1.0e-12, 1.0e-10 * lambda_max);
  const int64_t rank = values.gt(threshold).sum().item<int64_t>();
  const double smallest_retained =
      rank > 0 ? values[values.numel() - rank].item<double>() : 0.0;
  const double sum = values.clamp_min(0.0).sum().item<double>();
  double effective_rank = 0.0;
  if (sum > 0.0) {
    const auto probabilities = values.clamp_min(0.0) / sum;
    const auto positive = probabilities.masked_select(probabilities > 0.0);
    effective_rank =
        std::exp(-(positive * positive.log()).sum().item<double>());
  }

  const auto feature_std = centered.pow(2).mean(0).sqrt();
  const auto safe_std = torch::where(feature_std > 1.0e-12, feature_std,
                                     torch::ones_like(feature_std));
  auto correlation =
      (centered / safe_std).transpose(0, 1).matmul(centered / safe_std) /
      static_cast<double>(centered.size(0));
  std::vector<double> off_diagonal;
  off_diagonal.reserve(static_cast<std::size_t>(
      kDynamicFeatureCount * (kDynamicFeatureCount - 1) / 2));
  const auto corr = correlation.accessor<double, 2>();
  for (int64_t lhs = 0; lhs < correlation.size(0); ++lhs) {
    for (int64_t rhs = lhs + 1; rhs < correlation.size(1); ++rhs) {
      off_diagonal.push_back(std::fabs(corr[lhs][rhs]));
    }
  }
  std::sort(off_diagonal.begin(), off_diagonal.end());
  const auto p99_index = static_cast<std::size_t>(std::llround(
      0.99 * static_cast<double>(off_diagonal.size() - 1)));
  const auto target = target_input.to(torch::kFloat64).reshape({-1});
  const auto target_std =
      (target - target.mean()).pow(2).mean().sqrt().item<double>();
  return {.surface = surface,
          .edge = edge,
          .observations = x.size(0),
          .dimension = x.size(1),
          .retained_rank = rank,
          .nullity = x.size(1) - rank,
          .lambda_min = values[0].item<double>(),
          .lambda_q25 = ordered_quantile(values, 0.25),
          .lambda_median = ordered_quantile(values, 0.50),
          .lambda_q75 = ordered_quantile(values, 0.75),
          .lambda_max = lambda_max,
          .retained_condition =
              rank > 0 ? lambda_max / smallest_retained
                       : std::numeric_limits<double>::infinity(),
          .stable_rank = lambda_max > 0.0 ? sum / lambda_max : 0.0,
          .effective_rank = effective_rank,
          .correlation_max_abs_off_diagonal = off_diagonal.back(),
          .correlation_p99_abs_off_diagonal = off_diagonal[p99_index],
          .target_std = target_std,
          .psd_pass = values[0].item<double>() >= -threshold};
}

std::vector<GramDiagnostic>
make_gram_diagnostics(const Dataset &dataset,
                      const std::vector<int64_t> &fit_indices,
                      const Normalization &normalization) {
  const auto selected = index_tensor(fit_indices, torch::kCPU);
  const auto features = dataset.dynamic_features.index_select(0, selected);
  const auto target = dataset.target.index_select(0, selected);
  const auto standardized =
      (features - normalization.mean.view({1, 1, 1, -1})) *
      normalization.inv_std.view({1, 1, 1, -1});
  std::vector<GramDiagnostic> diagnostics;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    diagnostics.push_back(diagnose_gram(
        "raw", edge,
        features.select(1, edge).reshape({-1, kDynamicFeatureCount}),
        target.select(1, edge)));
    diagnostics.push_back(diagnose_gram(
        "shared_standardized", edge,
        standardized.select(1, edge).reshape({-1, kDynamicFeatureCount}),
        target.select(1, edge)));
  }
  return diagnostics;
}

PcaSpec make_pca_spec(const torch::Tensor &features,
                      const std::vector<int64_t> &fit_indices,
                      const Normalization &normalization) {
  torch::NoGradGuard no_grad;
  const auto selected = index_tensor(fit_indices, torch::kCPU);
  const auto fit =
      features.index_select(0, selected).to(torch::kFloat64);
  const auto standardized =
      (fit - normalization.mean.to(torch::kFloat64).view({1, 1, 1, -1})) *
      normalization.inv_std.to(torch::kFloat64).view({1, 1, 1, -1});
  std::vector<torch::Tensor> centers;
  std::vector<torch::Tensor> transforms;
  std::vector<int64_t> ranks;
  std::vector<double> diagonal_errors;
  std::vector<double> off_diagonal_errors;
  int64_t maximum_rank = 0;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto x = standardized.select(1, edge).reshape(
        {-1, kDynamicFeatureCount});
    const auto center = x.mean(0);
    const auto centered = x - center;
    auto covariance = centered.transpose(0, 1).matmul(centered) /
                      static_cast<double>(centered.size(0));
    covariance = 0.5 * (covariance + covariance.transpose(0, 1));
    auto [values, vectors] = at::linalg_eigh(covariance, "L");
    const double threshold =
        std::max(1.0e-12, 1.0e-10 * values[-1].item<double>());
    const int64_t rank = values.gt(threshold).sum().item<int64_t>();
    if (rank <= 0) {
      throw std::runtime_error("PCA whitening retained no directions");
    }
    const auto retained_values = values.narrow(0, values.numel() - rank, rank);
    const auto retained_vectors =
        vectors.narrow(1, vectors.size(1) - rank, rank);
    const auto transform =
        retained_vectors * retained_values.rsqrt().view({1, rank});
    const auto whitened = centered.matmul(transform);
    const auto whitened_covariance =
        whitened.transpose(0, 1).matmul(whitened) /
        static_cast<double>(whitened.size(0));
    const auto diagonal_error =
        (whitened_covariance.diagonal() - 1.0).abs().max().item<double>();
    auto off_diagonal = whitened_covariance.clone();
    off_diagonal.diagonal().zero_();
    centers.push_back(center);
    transforms.push_back(transform);
    ranks.push_back(rank);
    diagonal_errors.push_back(diagonal_error);
    off_diagonal_errors.push_back(off_diagonal.abs().max().item<double>());
    maximum_rank = std::max(maximum_rank, rank);
  }
  std::vector<torch::Tensor> padded;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    auto value = torch::zeros({kDynamicFeatureCount, maximum_rank},
                              torch::kFloat64);
    value.narrow(1, 0, ranks[static_cast<std::size_t>(edge)])
        .copy_(transforms[static_cast<std::size_t>(edge)]);
    padded.push_back(value);
  }
  return {.center = torch::stack(centers, 0).to(torch::kFloat32),
          .transform = torch::stack(padded, 0).to(torch::kFloat32),
          .ranks = std::move(ranks),
          .diagonal_max_abs_error = std::move(diagonal_errors),
          .off_diagonal_max_abs = std::move(off_diagonal_errors)};
}

std::string objective_name(ObjectiveKind kind) {
  switch (kind) {
  case ObjectiveKind::RawMse:
    return "raw_mse";
  case ObjectiveKind::ScaledHuber:
    return "100_times_huber_36x_beta_0.5";
  case ObjectiveKind::Direction:
    return "100_times_huber_36x_beta_0.5_plus_5_direction";
  case ObjectiveKind::Rank:
    return "100_times_huber_36x_beta_0.5_plus_5_direction_plus_5_rank";
  }
  throw std::runtime_error("unknown objective kind");
}

torch::Tensor parameter_vector(const TrainableAffine &head) {
  return torch::cat({head->weights.detach().reshape({-1}),
                     head->bias.detach().reshape({-1})});
}

double global_gradient_norm(const TrainableAffine &head) {
  auto sum = torch::zeros({}, head->weights.options());
  for (const auto &parameter : head->parameters()) {
    if (parameter.grad().defined()) {
      sum = sum + parameter.grad().pow(2).sum();
    }
  }
  return sum.sqrt().item<double>();
}

torch::Tensor selected_prediction(const TrainableAffine &head,
                                  const torch::Tensor &features,
                                  const std::vector<int64_t> &indices) {
  const auto selected = index_tensor(indices, features.device());
  return head->forward(features.index_select(0, selected));
}

struct GateResult {
  double direction_shortfall{std::numeric_limits<double>::infinity()};
  double rank_shortfall{std::numeric_limits<double>::infinity()};
  double rmse_excess{std::numeric_limits<double>::infinity()};
  bool pass{false};
};

GateResult validation_gate(const Metric &candidate, const Metric &oracle) {
  GateResult result;
  result.direction_shortfall =
      std::max(metric_direction(oracle) - metric_direction(candidate), 0.0);
  result.rank_shortfall =
      std::max(metric_rank(oracle) - metric_rank(candidate), 0.0);
  result.rmse_excess =
      std::max(metric_rmse(candidate) - metric_rmse(oracle), 0.0);
  result.pass = std::isfinite(result.direction_shortfall) &&
                std::isfinite(result.rank_shortfall) &&
                std::isfinite(result.rmse_excess) &&
                result.direction_shortfall <= kDirectionRankTolerance &&
                result.rank_shortfall <= kDirectionRankTolerance &&
                result.rmse_excess <= kRmseTolerance;
  return result;
}

struct Telemetry {
  int64_t step{0};
  double fit_total{0.0};
  double fit_raw_mse{0.0};
  double fit_scaled_huber{0.0};
  double fit_direction_loss{0.0};
  double fit_rank_loss{0.0};
  double validation_total{0.0};
  double validation_rmse{0.0};
  double validation_direction{0.0};
  double validation_rank{0.0};
  double parameter_l2_norm{0.0};
};

Telemetry snapshot(const TrainableAffine &head,
                   const torch::Tensor &features,
                   const torch::Tensor &target,
                   const std::vector<int64_t> &fit_indices,
                   const std::vector<int64_t> &validation_indices,
                   ObjectiveKind objective, int64_t step) {
  torch::NoGradGuard no_grad;
  const auto fit_prediction = selected_prediction(head, features, fit_indices);
  const auto fit_target = target.index_select(
      0, index_tensor(fit_indices, target.device()));
  const auto fit = compute_objective(fit_prediction, fit_target, objective);
  const auto validation_prediction =
      selected_prediction(head, features, validation_indices);
  const auto validation_target = target.index_select(
      0, index_tensor(validation_indices, target.device()));
  const auto validation =
      compute_objective(validation_prediction, validation_target, objective);
  const auto metric =
      evaluate_prediction(validation_prediction, validation_target);
  return {.step = step,
          .fit_total = fit.total.item<double>(),
          .fit_raw_mse = fit.raw_mse.item<double>(),
          .fit_scaled_huber = fit.scaled_huber.item<double>(),
          .fit_direction_loss = fit.direction.item<double>(),
          .fit_rank_loss = fit.rank.item<double>(),
          .validation_total = validation.total.item<double>(),
          .validation_rmse = metric_rmse(metric),
          .validation_direction = metric_direction(metric),
          .validation_rank = metric_rank(metric),
          .parameter_l2_norm = parameter_vector(head).norm().item<double>()};
}

bool telemetry_checkpoint(int64_t step) {
  static const std::set<int64_t> checkpoints{0,   10,   100,  500,
                                              1000, 2000, 3500};
  return checkpoints.contains(step);
}

struct TrainResult {
  std::string name;
  std::string optimizer;
  ObjectiveKind objective{ObjectiveKind::RawMse};
  bool full_batch{false};
  bool pca_whitened{false};
  bool pca_validation_pass{true};
  bool pca_mapped_prediction_pass{true};
  double clip_norm{0.0};
  int64_t optimizer_step_index{0};
  int64_t configured_max_iterations{0};
  int64_t configured_max_evaluations{0};
  int64_t actual_lbfgs_iterations{0};
  int64_t closure_evaluations{0};
  int64_t clip_count{0};
  double initial_full_gradient_norm{0.0};
  double final_full_gradient_norm{0.0};
  double preclip_gradient_norm_mean{0.0};
  double preclip_gradient_norm_max{0.0};
  double preclip_gradient_norm_final{0.0};
  double update_norm_mean{0.0};
  double update_norm_max{0.0};
  double update_norm_final{0.0};
  double pca_mapped_prediction_max_abs_delta{0.0};
  std::vector<double> pca_cuda_diagonal_max_abs_error;
  std::vector<double> pca_cuda_off_diagonal_max_abs;
  std::vector<int64_t> pca_ranks;
  std::vector<Telemetry> telemetry;
  Metric fit_metric;
  Metric validation_metric;
  GateResult gate;
  std::string zero_state_digest;
  std::string final_state_digest;
  std::string validation_prediction_digest;
  ModelState final_state;
  TrainableAffine head;
};

void require_finite_training_result(const TrainResult &result) {
  const std::vector<double> values{
      result.initial_full_gradient_norm,
      result.final_full_gradient_norm,
      result.preclip_gradient_norm_mean,
      result.preclip_gradient_norm_max,
      result.preclip_gradient_norm_final,
      result.update_norm_mean,
      result.update_norm_max,
      result.update_norm_final,
      metric_rmse(result.fit_metric),
      metric_direction(result.fit_metric),
      metric_rank(result.fit_metric),
      metric_correlation(result.fit_metric),
      metric_rmse(result.validation_metric),
      metric_direction(result.validation_metric),
      metric_rank(result.validation_metric),
      metric_correlation(result.validation_metric),
      result.gate.direction_shortfall,
      result.gate.rank_shortfall,
      result.gate.rmse_excess,
      result.pca_mapped_prediction_max_abs_delta};
  if (std::any_of(values.begin(), values.end(),
                  [](double value) { return !std::isfinite(value); })) {
    throw std::runtime_error("training result contains a non-finite scalar");
  }
  for (const auto &telemetry : result.telemetry) {
    const std::vector<double> telemetry_values{
        telemetry.fit_total,
        telemetry.fit_raw_mse,
        telemetry.fit_scaled_huber,
        telemetry.fit_direction_loss,
        telemetry.fit_rank_loss,
        telemetry.validation_total,
        telemetry.validation_rmse,
        telemetry.validation_direction,
        telemetry.validation_rank,
        telemetry.parameter_l2_norm};
    if (std::any_of(telemetry_values.begin(), telemetry_values.end(),
                    [](double value) { return !std::isfinite(value); })) {
      throw std::runtime_error("training telemetry contains a non-finite scalar");
    }
  }
}

std::pair<std::vector<double>, std::vector<double>>
validate_pca_cuda_float32(const TrainableAffine &head,
                          const torch::Tensor &features,
                          const std::vector<int64_t> &fit_indices,
                          const std::vector<int64_t> &ranks) {
  torch::NoGradGuard no_grad;
  const auto selected = index_tensor(fit_indices, features.device());
  auto standardized =
      (features.index_select(0, selected) - head->feature_mean) *
      head->feature_inv_std;
  standardized = standardized - head->pca_center;
  std::vector<double> diagonal_errors;
  std::vector<double> off_diagonal_errors;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto rank = ranks[static_cast<std::size_t>(edge)];
    const auto reduced =
        standardized.select(1, edge)
            .reshape({-1, kDynamicFeatureCount})
            .matmul(head->pca_transform.select(0, edge).narrow(1, 0, rank));
    const auto covariance = reduced.transpose(0, 1).matmul(reduced) /
                            static_cast<double>(reduced.size(0));
    diagonal_errors.push_back(
        (covariance.diagonal() - 1.0).abs().max().item<double>());
    auto off_diagonal = covariance.clone();
    off_diagonal.diagonal().zero_();
    off_diagonal_errors.push_back(
        off_diagonal.abs().max().item<double>());
  }
  return {std::move(diagonal_errors), std::move(off_diagonal_errors)};
}

TrainResult train_adam(
    const std::string &name, const Dataset &dataset,
    const torch::Tensor &features_device, const torch::Tensor &target_device,
    const std::vector<int64_t> &fit_indices,
    const std::vector<int64_t> &validation_indices,
    const BatchSchedule &schedule, ObjectiveKind objective, bool full_batch,
    bool pca_whitened, double clip_norm, const Metric &oracle_metric,
    const torch::Device &device) {
  if (!full_batch && static_cast<int64_t>(schedule.batches.size()) != kSteps) {
    throw std::runtime_error("minibatch arm received an invalid schedule");
  }
  const auto normalization =
      compute_normalization(dataset.dynamic_features, fit_indices);
  std::optional<PcaSpec> pca;
  if (pca_whitened) {
    pca = make_pca_spec(dataset.dynamic_features, fit_indices, normalization);
  }
  auto head = std::make_shared<TrainableAffineImpl>(normalization, pca);
  head->to(device, torch::kFloat32);
  TrainResult result;
  result.name = name;
  result.optimizer = "adam";
  result.objective = objective;
  result.full_batch = full_batch;
  result.pca_whitened = pca_whitened;
  result.clip_norm = clip_norm;
  result.configured_max_iterations = kSteps;
  result.head = head;
  result.zero_state_digest = state_digest(head->mapped_state());
  if (pca.has_value()) {
    result.pca_ranks = pca->ranks;
    auto [diagonal, off_diagonal] = validate_pca_cuda_float32(
        head, features_device, fit_indices, pca->ranks);
    result.pca_cuda_diagonal_max_abs_error = std::move(diagonal);
    result.pca_cuda_off_diagonal_max_abs = std::move(off_diagonal);
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      const auto diagonal_error = result.pca_cuda_diagonal_max_abs_error
          [static_cast<std::size_t>(edge)];
      const auto off_diagonal_error = result.pca_cuda_off_diagonal_max_abs
          [static_cast<std::size_t>(edge)];
      result.pca_validation_pass =
          result.pca_validation_pass && std::isfinite(diagonal_error) &&
          std::isfinite(off_diagonal_error) &&
          diagonal_error <= kPcaWhiteningTolerance &&
          off_diagonal_error <= kPcaWhiteningTolerance;
    }
  }
  result.telemetry.push_back(snapshot(head, features_device, target_device,
                                      fit_indices, validation_indices,
                                      objective, 0));

  torch::optim::Adam optimizer(head->parameters(),
                               torch::optim::AdamOptions(kLearningRate));
  optimizer.zero_grad();
  {
    const auto prediction =
        selected_prediction(head, features_device, fit_indices);
    const auto realized = target_device.index_select(
        0, index_tensor(fit_indices, target_device.device()));
    compute_objective(prediction, realized, objective).total.backward();
    result.initial_full_gradient_norm = global_gradient_norm(head);
    optimizer.zero_grad();
  }

  double preclip_sum = 0.0;
  double update_sum = 0.0;
  for (int64_t step = 0; step < kSteps; ++step) {
    std::vector<int64_t> batch_indices;
    if (full_batch) {
      batch_indices = fit_indices;
    } else {
      const auto &local_batch =
          schedule.batches[static_cast<std::size_t>(step)];
      batch_indices.reserve(local_batch.size());
      for (const auto local : local_batch) {
        batch_indices.push_back(
            fit_indices[static_cast<std::size_t>(local)]);
      }
    }
    const auto selected = index_tensor(batch_indices, features_device.device());
    const auto batch_features = features_device.index_select(0, selected);
    const auto batch_target = target_device.index_select(0, selected);
    const auto before = parameter_vector(head).clone();
    optimizer.zero_grad();
    const auto loss =
        compute_objective(head->forward(batch_features), batch_target, objective)
            .total;
    if (!torch::isfinite(loss).item<bool>()) {
      throw std::runtime_error("Adam arm produced a non-finite loss");
    }
    loss.backward();
    const double preclip = global_gradient_norm(head);
    if (!std::isfinite(preclip)) {
      throw std::runtime_error("Adam produced a non-finite gradient norm");
    }
    result.preclip_gradient_norm_final = preclip;
    result.preclip_gradient_norm_max =
        std::max(result.preclip_gradient_norm_max, preclip);
    preclip_sum += preclip;
    if (clip_norm > 0.0) {
      if (preclip > clip_norm) {
        ++result.clip_count;
      }
      torch::nn::utils::clip_grad_norm_(head->parameters(), clip_norm);
    }
    optimizer.step();
    const double update = (parameter_vector(head) - before).norm().item<double>();
    if (!std::isfinite(update)) {
      throw std::runtime_error("Adam produced a non-finite update norm");
    }
    result.update_norm_final = update;
    result.update_norm_max = std::max(result.update_norm_max, update);
    update_sum += update;
    const auto completed = step + 1;
    if (telemetry_checkpoint(completed)) {
      result.telemetry.push_back(snapshot(
          head, features_device, target_device, fit_indices,
          validation_indices, objective, completed));
    }
  }
  result.optimizer_step_index = kSteps;
  result.preclip_gradient_norm_mean = preclip_sum / kSteps;
  result.update_norm_mean = update_sum / kSteps;

  optimizer.zero_grad();
  const auto fit_prediction =
      selected_prediction(head, features_device, fit_indices);
  const auto fit_target = target_device.index_select(
      0, index_tensor(fit_indices, target_device.device()));
  compute_objective(fit_prediction, fit_target, objective).total.backward();
  result.final_full_gradient_norm = global_gradient_norm(head);
  optimizer.zero_grad();

  torch::NoGradGuard no_grad;
  const auto validation_prediction =
      selected_prediction(head, features_device, validation_indices);
  const auto validation_target = target_device.index_select(
      0, index_tensor(validation_indices, target_device.device()));
  result.fit_metric = evaluate_prediction(fit_prediction, fit_target);
  result.validation_metric =
      evaluate_prediction(validation_prediction, validation_target);
  result.gate = validation_gate(result.validation_metric, oracle_metric);
  result.gate.pass = result.gate.pass && result.pca_validation_pass;
  result.final_state = head->mapped_state();
  result.final_state_digest = state_digest(result.final_state);
  result.validation_prediction_digest =
      prediction_digest(validation_prediction);

  if (pca_whitened) {
    auto ordinary = std::make_shared<TrainableAffineImpl>(normalization,
                                                          std::nullopt);
    ordinary->to(device, torch::kFloat32);
    ordinary->weights.copy_(result.final_state.weights.to(device));
    ordinary->bias.copy_(result.final_state.bias.to(device));
    const auto direct = head->forward(features_device);
    const auto mapped = ordinary->forward(features_device);
    result.pca_mapped_prediction_max_abs_delta =
        (direct - mapped).abs().max().item<double>();
    if (!std::isfinite(result.pca_mapped_prediction_max_abs_delta)) {
      throw std::runtime_error(
          "PCA mapped-head prediction comparison is non-finite");
    }
    result.pca_mapped_prediction_pass =
        result.pca_mapped_prediction_max_abs_delta <= 2.0e-5;
    result.pca_validation_pass =
        result.pca_validation_pass && result.pca_mapped_prediction_pass;
    result.gate.pass = result.gate.pass && result.pca_mapped_prediction_pass;
  }
  require_finite_training_result(result);
  return result;
}

std::string bool_text(bool value) { return value ? "true" : "false"; }

void emit_metric(std::ostream &output, const std::string &prefix,
                 const Metric &metric) {
  output << prefix << ".count=" << metric.count << '\n';
  output << prefix << ".pair_count=" << metric.pair_count << '\n';
  output << prefix << ".rmse=" << metric_rmse(metric) << '\n';
  output << prefix << ".mae="
         << (metric.count > 0
                 ? metric.abs_error / static_cast<double>(metric.count)
                 : 0.0)
         << '\n';
  output << prefix << ".directional_accuracy=" << metric_direction(metric)
         << '\n';
  output << prefix << ".pairwise_rank_accuracy=" << metric_rank(metric)
         << '\n';
  output << prefix << ".correlation=" << metric_correlation(metric) << '\n';
}

std::string batch_text(const std::vector<int64_t> &batch) {
  std::ostringstream output;
  for (std::size_t index = 0; index < batch.size(); ++index) {
    if (index != 0) {
      output << ',';
    }
    output << batch[index];
  }
  return output.str();
}

void emit_schedule(std::ostream &output, const std::string &prefix,
                   const BatchSchedule &schedule) {
  const auto sample_count = std::accumulate(
      schedule.batches.begin(), schedule.batches.end(), int64_t{0},
      [](int64_t total, const std::vector<int64_t> &batch) {
        return total + static_cast<int64_t>(batch.size());
      });
  const auto [minimum_visit, maximum_visit] = std::minmax_element(
      schedule.visit_counts.begin(), schedule.visit_counts.end());
  std::map<int64_t, int64_t> visit_histogram;
  for (const auto visits : schedule.visit_counts) {
    ++visit_histogram[visits];
  }
  output << prefix << ".kind=production_random_sampler_epoch_reseeded\n";
  output << prefix << ".seed=" << kScheduleSeed << '\n';
  output << prefix << ".batch_count=" << schedule.batches.size() << '\n';
  output << prefix << ".epoch_count=" << schedule.epoch_count << '\n';
  output << prefix << ".sample_count=" << sample_count << '\n';
  output << prefix << ".digest=" << schedule.digest << '\n';
  output << prefix << ".visit_min=" << *minimum_visit << '\n';
  output << prefix << ".visit_max=" << *maximum_visit << '\n';
  output << prefix << ".batch_size_histogram=";
  bool first = true;
  for (const auto &[size, count] : schedule.batch_size_histogram) {
    output << (first ? "" : ",") << size << ':' << count;
    first = false;
  }
  output << '\n';
  output << prefix << ".visit_histogram=";
  first = true;
  for (const auto &[visits, anchors] : visit_histogram) {
    output << (first ? "" : ",") << visits << ':' << anchors;
    first = false;
  }
  output << '\n';
  output << prefix << ".batch.0=" << batch_text(schedule.batches.front())
         << '\n';
  output << prefix << ".batch.1=" << batch_text(schedule.batches[1]) << '\n';
  output << prefix << ".batch.last=" << batch_text(schedule.batches.back())
         << '\n';
}

void emit_arm(std::ostream &output, const std::string &prefix,
              const TrainResult &result, bool selection_eligible) {
  output << prefix << ".applicable=true\n";
  output << prefix << ".selection_eligible="
         << bool_text(selection_eligible) << '\n';
  output << prefix << ".objective=" << objective_name(result.objective)
         << '\n';
  output << prefix << ".optimizer=" << result.optimizer << '\n';
  output << prefix << ".full_batch=" << bool_text(result.full_batch) << '\n';
  output << prefix << ".pca_whitened=" << bool_text(result.pca_whitened)
         << '\n';
  output << prefix << ".pca_validation_pass="
         << bool_text(result.pca_validation_pass) << '\n';
  output << prefix << ".pca_mapped_prediction_pass="
         << bool_text(result.pca_mapped_prediction_pass) << '\n';
  output << prefix << ".clip_norm=" << result.clip_norm << '\n';
  output << prefix << ".configured_max_iterations="
         << result.configured_max_iterations << '\n';
  output << prefix << ".configured_max_evaluations="
         << result.configured_max_evaluations << '\n';
  output << prefix << ".optimizer_outer_step_count="
         << result.optimizer_step_index << '\n';
  output << prefix << ".actual_lbfgs_iterations="
         << result.actual_lbfgs_iterations << '\n';
  output << prefix << ".closure_evaluations=" << result.closure_evaluations
         << '\n';
  output << prefix << ".clip_count=" << result.clip_count << '\n';
  output << prefix << ".initial_full_gradient_norm="
         << result.initial_full_gradient_norm << '\n';
  output << prefix << ".final_full_gradient_norm="
         << result.final_full_gradient_norm << '\n';
  output << prefix << ".preclip_gradient_norm_mean="
         << result.preclip_gradient_norm_mean << '\n';
  output << prefix << ".preclip_gradient_norm_max="
         << result.preclip_gradient_norm_max << '\n';
  output << prefix << ".preclip_gradient_norm_final="
         << result.preclip_gradient_norm_final << '\n';
  output << prefix << ".update_norm_mean=" << result.update_norm_mean << '\n';
  output << prefix << ".update_norm_max=" << result.update_norm_max << '\n';
  output << prefix << ".update_norm_final=" << result.update_norm_final
         << '\n';
  output << prefix << ".zero_state_digest=" << result.zero_state_digest
         << '\n';
  output << prefix << ".final_state_digest=" << result.final_state_digest
         << '\n';
  output << prefix << ".validation_prediction_digest="
         << result.validation_prediction_digest << '\n';
  output << prefix << ".pca_mapped_prediction_max_abs_delta="
         << result.pca_mapped_prediction_max_abs_delta << '\n';
  for (std::size_t edge = 0; edge < result.pca_ranks.size(); ++edge) {
    output << prefix << ".pca.edge." << edge
           << ".rank=" << result.pca_ranks[edge] << '\n';
    output << prefix << ".pca.edge." << edge
           << ".cuda_float32_diagonal_max_abs_error="
           << result.pca_cuda_diagonal_max_abs_error[edge] << '\n';
    output << prefix << ".pca.edge." << edge
           << ".cuda_float32_off_diagonal_max_abs="
           << result.pca_cuda_off_diagonal_max_abs[edge] << '\n';
  }
  emit_metric(output, prefix + ".fit", result.fit_metric);
  emit_metric(output, prefix + ".validation", result.validation_metric);
  output << prefix << ".gate.direction_shortfall="
         << result.gate.direction_shortfall << '\n';
  output << prefix << ".gate.rank_shortfall=" << result.gate.rank_shortfall
         << '\n';
  output << prefix << ".gate.rmse_excess=" << result.gate.rmse_excess << '\n';
  output << prefix << ".validation_parity_pass="
         << bool_text(result.gate.pass) << '\n';
  for (const auto &telemetry : result.telemetry) {
    const auto telemetry_prefix =
        prefix + ".telemetry." + std::to_string(telemetry.step);
    output << telemetry_prefix << ".fit_total=" << telemetry.fit_total << '\n';
    output << telemetry_prefix << ".fit_raw_mse=" << telemetry.fit_raw_mse
           << '\n';
    output << telemetry_prefix << ".fit_scaled_huber="
           << telemetry.fit_scaled_huber << '\n';
    output << telemetry_prefix << ".fit_direction_loss="
           << telemetry.fit_direction_loss << '\n';
    output << telemetry_prefix << ".fit_rank_loss="
           << telemetry.fit_rank_loss << '\n';
    output << telemetry_prefix << ".validation_total="
           << telemetry.validation_total << '\n';
    output << telemetry_prefix << ".validation_rmse="
           << telemetry.validation_rmse << '\n';
    output << telemetry_prefix << ".validation_direction="
           << telemetry.validation_direction << '\n';
    output << telemetry_prefix << ".validation_rank="
           << telemetry.validation_rank << '\n';
    output << telemetry_prefix << ".parameter_l2_norm="
           << telemetry.parameter_l2_norm << '\n';
  }
}

void emit_skipped_arm(std::ostream &output, const std::string &prefix,
                      const std::string &reason) {
  output << prefix << ".applicable=false\n";
  output << prefix << ".selection_eligible=true\n";
  output << prefix << ".reason=" << reason << '\n';
}

void emit_gram_diagnostics(
    std::ostream &output, const std::vector<GramDiagnostic> &diagnostics) {
  for (const auto &diagnostic : diagnostics) {
    const auto prefix = "conditioning." + diagnostic.surface + ".edge." +
                        std::to_string(diagnostic.edge);
    output << prefix << ".observations=" << diagnostic.observations << '\n';
    output << prefix << ".dimension=" << diagnostic.dimension << '\n';
    output << prefix << ".retained_rank=" << diagnostic.retained_rank << '\n';
    output << prefix << ".nullity=" << diagnostic.nullity << '\n';
    output << prefix << ".lambda_min=" << diagnostic.lambda_min << '\n';
    output << prefix << ".lambda_q25=" << diagnostic.lambda_q25 << '\n';
    output << prefix << ".lambda_median=" << diagnostic.lambda_median << '\n';
    output << prefix << ".lambda_q75=" << diagnostic.lambda_q75 << '\n';
    output << prefix << ".lambda_max=" << diagnostic.lambda_max << '\n';
    output << prefix << ".retained_condition="
           << diagnostic.retained_condition << '\n';
    output << prefix << ".stable_rank=" << diagnostic.stable_rank << '\n';
    output << prefix << ".effective_rank=" << diagnostic.effective_rank
           << '\n';
    output << prefix << ".correlation_max_abs_off_diagonal="
           << diagnostic.correlation_max_abs_off_diagonal << '\n';
    output << prefix << ".correlation_p99_abs_off_diagonal="
           << diagnostic.correlation_p99_abs_off_diagonal << '\n';
    output << prefix << ".target_std=" << diagnostic.target_std << '\n';
    output << prefix << ".psd_pass=" << bool_text(diagnostic.psd_pass) << '\n';
  }
}

struct PredictionComparison {
  bool torch_equal{false};
  double maximum_abs_delta{std::numeric_limits<double>::infinity()};
  double mean_abs_delta{std::numeric_limits<double>::infinity()};
};

PredictionComparison compare_predictions(const torch::Tensor &lhs_input,
                                         const torch::Tensor &rhs_input) {
  const auto lhs = lhs_input.detach().to(torch::kCPU, torch::kFloat32);
  const auto rhs = rhs_input.detach().to(torch::kCPU, torch::kFloat32);
  if (lhs.sizes() != rhs.sizes()) {
    throw std::runtime_error("prediction comparison shape mismatch");
  }
  if (!torch::isfinite(lhs).all().item<bool>() ||
      !torch::isfinite(rhs).all().item<bool>()) {
    throw std::runtime_error("prediction comparison contains non-finite data");
  }
  const auto delta = (lhs - rhs).abs();
  return {.torch_equal = torch::equal(lhs, rhs),
          .maximum_abs_delta = delta.max().item<double>(),
          .mean_abs_delta = delta.mean().item<double>()};
}

void write_probe_atomic(const std::filesystem::path &output_path,
                        const std::string &contents) {
  const auto temporary = std::filesystem::path(output_path.string() + ".tmp");
  if (std::filesystem::exists(output_path) ||
      std::filesystem::exists(temporary)) {
    throw std::runtime_error(
        "refusing to replace an existing result or temporary probe");
  }
  if (!output_path.parent_path().empty()) {
    std::filesystem::create_directories(output_path.parent_path());
  }
  const int descriptor =
      ::open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
  if (descriptor < 0) {
    throw std::runtime_error("failed to exclusively create result temporary");
  }
  bool installed = false;
  try {
    std::size_t offset = 0;
    while (offset < contents.size()) {
      const auto written = ::write(descriptor, contents.data() + offset,
                                   contents.size() - offset);
      if (written < 0) {
        if (errno == EINTR) {
          continue;
        }
        throw std::runtime_error("failed while writing result probe");
      }
      if (written == 0) {
        throw std::runtime_error("short write while writing result probe");
      }
      offset += static_cast<std::size_t>(written);
    }
    if (::fsync(descriptor) != 0) {
      throw std::runtime_error("failed to synchronize result probe");
    }
    if (::close(descriptor) != 0) {
      throw std::runtime_error("failed to close result probe");
    }
    if (::link(temporary.c_str(), output_path.c_str()) != 0) {
      throw std::runtime_error(
          "failed to atomically install result without replacement");
    }
    installed = true;
  } catch (...) {
    (void)::close(descriptor);
    (void)::unlink(temporary.c_str());
    throw;
  }
  if (!installed || ::unlink(temporary.c_str()) != 0) {
    throw std::runtime_error("failed to remove installed result temporary");
  }
}

TrainResult train_lbfgs(
    const std::string &name, const Dataset &dataset,
    const torch::Tensor &features_device, const torch::Tensor &target_device,
    const std::vector<int64_t> &fit_indices,
    const std::vector<int64_t> &validation_indices, const Metric &oracle_metric,
    const torch::Device &device) {
  const auto normalization =
      compute_normalization(dataset.dynamic_features, fit_indices);
  auto head = std::make_shared<TrainableAffineImpl>(normalization, std::nullopt);
  head->to(device, torch::kFloat32);
  TrainResult result;
  result.name = name;
  result.optimizer = "lbfgs_strong_wolfe";
  result.objective = ObjectiveKind::RawMse;
  result.full_batch = true;
  result.clip_norm = 0.0;
  result.configured_max_iterations = 500;
  result.configured_max_evaluations = 625;
  result.head = head;
  result.zero_state_digest = state_digest(head->mapped_state());
  result.telemetry.push_back(snapshot(
      head, features_device, target_device, fit_indices, validation_indices,
      ObjectiveKind::RawMse, 0));

  const auto fit_selected = index_tensor(fit_indices, features_device.device());
  const auto fit_features = features_device.index_select(0, fit_selected);
  const auto fit_target = target_device.index_select(0, fit_selected);
  torch::optim::LBFGSOptions lbfgs_options(1.0);
  lbfgs_options.max_iter(500);
  lbfgs_options.max_eval(625);
  lbfgs_options.tolerance_grad(1.0e-12);
  lbfgs_options.tolerance_change(1.0e-15);
  lbfgs_options.history_size(100);
  lbfgs_options.line_search_fn(std::optional<std::string>{"strong_wolfe"});
  torch::optim::LBFGS optimizer(head->parameters(), lbfgs_options);

  optimizer.zero_grad();
  compute_objective(head->forward(fit_features), fit_target,
                    ObjectiveKind::RawMse)
      .total.backward();
  result.initial_full_gradient_norm = global_gradient_norm(head);
  optimizer.zero_grad();
  const auto before = parameter_vector(head).clone();
  auto closure = [&]() {
    optimizer.zero_grad();
    const auto loss =
        compute_objective(head->forward(fit_features), fit_target,
                          ObjectiveKind::RawMse)
            .total;
    if (!torch::isfinite(loss).item<bool>()) {
      throw std::runtime_error("LBFGS produced a non-finite loss");
    }
    loss.backward();
    ++result.closure_evaluations;
    return loss;
  };
  optimizer.step(closure);
  result.optimizer_step_index = 1;
  if (!optimizer.state().empty()) {
    const auto *state = dynamic_cast<const torch::optim::LBFGSParamState *>(
        optimizer.state().begin()->second.get());
    if (state != nullptr) {
      result.actual_lbfgs_iterations = state->n_iter();
    }
  }
  const auto update = (parameter_vector(head) - before).norm().item<double>();
  result.update_norm_mean = update;
  result.update_norm_max = update;
  result.update_norm_final = update;

  optimizer.zero_grad();
  const auto fit_prediction = head->forward(fit_features);
  compute_objective(fit_prediction, fit_target, ObjectiveKind::RawMse)
      .total.backward();
  result.final_full_gradient_norm = global_gradient_norm(head);
  optimizer.zero_grad();
  result.telemetry.push_back(snapshot(
      head, features_device, target_device, fit_indices, validation_indices,
      ObjectiveKind::RawMse, 1));

  torch::NoGradGuard no_grad;
  const auto validation_prediction =
      selected_prediction(head, features_device, validation_indices);
  const auto validation_target = target_device.index_select(
      0, index_tensor(validation_indices, target_device.device()));
  result.fit_metric = evaluate_prediction(fit_prediction, fit_target);
  result.validation_metric =
      evaluate_prediction(validation_prediction, validation_target);
  result.gate = validation_gate(result.validation_metric, oracle_metric);
  result.final_state = head->mapped_state();
  result.final_state_digest = state_digest(result.final_state);
  result.validation_prediction_digest =
      prediction_digest(validation_prediction);
  require_finite_training_result(result);
  return result;
}

struct RuntimeIdentity {
  int cuda_device_count{0};
  int cuda_runtime_version{0};
  int cuda_driver_version{0};
  std::string cuda_device_name;
  int cuda_compute_major{0};
  int cuda_compute_minor{0};
  std::uint64_t cuda_total_global_memory{0};
};

void require_cuda_success(cudaError_t status, const char *operation) {
  if (status != cudaSuccess) {
    throw std::runtime_error(std::string("CUDA runtime operation failed: ") +
                             operation);
  }
}

std::string required_environment(const char *name,
                                 const std::string &expected) {
  const char *value = std::getenv(name);
  if (value == nullptr || value != expected) {
    throw std::runtime_error(std::string("required deterministic environment is missing: ") +
                             name);
  }
  return value;
}

RuntimeIdentity configure_deterministic_runtime() {
  (void)required_environment("CUBLAS_WORKSPACE_CONFIG", ":4096:8");
  (void)required_environment("CUDA_DEVICE_ORDER", "PCI_BUS_ID");
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  auto &context = at::globalContext();
  context.setDeterministicAlgorithms(true, false);
  context.setDeterministicCuDNN(true);
  context.setBenchmarkCuDNN(false);
  context.setAllowTF32CuBLAS(false);
  context.setAllowTF32CuDNN(false);
  context.setDeterministicFillUninitializedMemory(true);

  RuntimeIdentity identity;
  require_cuda_success(cudaGetDeviceCount(&identity.cuda_device_count),
                       "cudaGetDeviceCount");
  if (identity.cuda_device_count <= 0 || !torch::cuda::is_available()) {
    throw std::runtime_error("a CUDA device is required");
  }
  require_cuda_success(cudaSetDevice(0), "cudaSetDevice");
  require_cuda_success(cudaRuntimeGetVersion(&identity.cuda_runtime_version),
                       "cudaRuntimeGetVersion");
  require_cuda_success(cudaDriverGetVersion(&identity.cuda_driver_version),
                       "cudaDriverGetVersion");
  cudaDeviceProp properties{};
  require_cuda_success(cudaGetDeviceProperties(&properties, 0),
                       "cudaGetDeviceProperties");
  identity.cuda_device_name = properties.name;
  if (identity.cuda_device_name.empty() ||
      identity.cuda_device_name.find_first_of("\r\n=") != std::string::npos) {
    throw std::runtime_error("CUDA device name is unsafe for probe emission");
  }
  identity.cuda_compute_major = properties.major;
  identity.cuda_compute_minor = properties.minor;
  identity.cuda_total_global_memory =
      static_cast<std::uint64_t>(properties.totalGlobalMem);
  torch::manual_seed(kSeed);
  torch::cuda::manual_seed_all(kSeed);
  require_cuda_success(cudaDeviceSynchronize(), "cudaDeviceSynchronize");
  return identity;
}

void reset_deterministic_seed() {
  torch::manual_seed(kSeed);
  torch::cuda::manual_seed_all(kSeed);
}

bool model_state_equal(const ModelState &lhs, const ModelState &rhs) {
  return torch::equal(lhs.mean.to(torch::kCPU, torch::kFloat32),
                      rhs.mean.to(torch::kCPU, torch::kFloat32)) &&
         torch::equal(lhs.inv_std.to(torch::kCPU, torch::kFloat32),
                      rhs.inv_std.to(torch::kCPU, torch::kFloat32)) &&
         torch::equal(lhs.weights.to(torch::kCPU, torch::kFloat32),
                      rhs.weights.to(torch::kCPU, torch::kFloat32)) &&
         torch::equal(lhs.bias.to(torch::kCPU, torch::kFloat32),
                      rhs.bias.to(torch::kCPU, torch::kFloat32));
}

bool sidecar_parameters_frozen(const mdn::PerEdgeAffineReturnHead &head) {
  const auto parameters = head->parameters();
  return std::all_of(parameters.begin(), parameters.end(),
                     [](const torch::Tensor &parameter) {
                       return !parameter.requires_grad();
                     });
}

bool trainable_affine_parameters_frozen(const TrainableAffine &head) {
  const auto parameters = head->parameters();
  return std::all_of(parameters.begin(), parameters.end(),
                     [](const torch::Tensor &parameter) {
                       return !parameter.requires_grad();
                     });
}

void emit_comparison(std::ostream &output, const std::string &prefix,
                     const PredictionComparison &comparison) {
  output << prefix << ".torch_equal=" << bool_text(comparison.torch_equal)
         << '\n';
  output << prefix << ".maximum_abs_delta=" << comparison.maximum_abs_delta
         << '\n';
  output << prefix << ".mean_abs_delta=" << comparison.mean_abs_delta << '\n';
}

void emit_adjacent_delta(std::ostream &output, const std::string &prefix,
                         const Metric &before, const Metric &after) {
  output << prefix << ".rmse=" << metric_rmse(after) - metric_rmse(before)
         << '\n';
  output << prefix << ".directional_accuracy="
         << metric_direction(after) - metric_direction(before) << '\n';
  output << prefix << ".pairwise_rank_accuracy="
         << metric_rank(after) - metric_rank(before) << '\n';
}

ModelState remap_oracle_to_candidate_normalization(
    const ModelState &oracle, const Normalization &candidate) {
  torch::NoGradGuard no_grad;
  const auto oracle_mean = oracle.mean.to(torch::kCPU, torch::kFloat64);
  const auto oracle_inv_std =
      oracle.inv_std.to(torch::kCPU, torch::kFloat64);
  const auto oracle_weights =
      oracle.weights.to(torch::kCPU, torch::kFloat64);
  const auto oracle_bias = oracle.bias.to(torch::kCPU, torch::kFloat64);
  const auto candidate_mean =
      candidate.mean.to(torch::kCPU, torch::kFloat64);
  const auto candidate_inv_std =
      candidate.inv_std.to(torch::kCPU, torch::kFloat64);
  if (!(candidate_inv_std > 0.0).all().item<bool>()) {
    throw std::runtime_error("candidate normalization contains a nonpositive scale");
  }
  const auto raw_slope =
      oracle_weights * oracle_inv_std.view({1, kDynamicFeatureCount});
  const auto candidate_weights =
      raw_slope / candidate_inv_std.view({1, kDynamicFeatureCount});
  const auto candidate_bias =
      oracle_bias -
      (raw_slope * oracle_mean.view({1, kDynamicFeatureCount})).sum(1) +
      (raw_slope * candidate_mean.view({1, kDynamicFeatureCount})).sum(1);
  const std::vector<torch::Tensor> tensors{candidate_mean, candidate_inv_std,
                                           candidate_weights, candidate_bias};
  if (std::any_of(tensors.begin(), tensors.end(), [](const torch::Tensor &value) {
        return !torch::isfinite(value).all().item<bool>();
      })) {
    throw std::runtime_error("remapped oracle state is non-finite");
  }
  return {.mean = candidate_mean.to(torch::kFloat32).contiguous(),
          .inv_std = candidate_inv_std.to(torch::kFloat32).contiguous(),
          .weights = candidate_weights.to(torch::kFloat32).contiguous(),
          .bias = candidate_bias.to(torch::kFloat32).contiguous()};
}

TrainResult refit_selected_candidate(
    const TrainResult &selected, const Dataset &development,
    const torch::Tensor &features_device, const torch::Tensor &target_device,
    const std::vector<int64_t> &refit_indices,
    const std::vector<int64_t> &telemetry_indices,
    const BatchSchedule &refit_schedule, const Metric &oracle_metric,
    const torch::Device &device) {
  reset_deterministic_seed();
  if (selected.optimizer == "lbfgs_strong_wolfe") {
    return train_lbfgs(selected.name + "_refit", development, features_device,
                       target_device, refit_indices, telemetry_indices,
                       oracle_metric, device);
  }
  if (selected.optimizer != "adam") {
    throw std::runtime_error("selected candidate has an unknown optimizer");
  }
  return train_adam(selected.name + "_refit", development, features_device,
                    target_device, refit_indices, telemetry_indices,
                    refit_schedule, selected.objective, selected.full_batch,
                    selected.pca_whitened, selected.clip_norm, oracle_metric,
                    device);
}

} // namespace

int main(int argc, char **argv) {
  try {
    std::locale::global(std::locale::classic());
    const auto options = parse_options(argc, argv);
    if (options.schema_id != kExpectedResultSchema) {
      throw std::runtime_error("result schema does not match the frozen contract");
    }
    const auto output_temporary =
        std::filesystem::path(options.output.string() + ".tmp");
    if (std::filesystem::exists(options.output) ||
        std::filesystem::exists(output_temporary)) {
      throw std::runtime_error(
          "refusing to run with an existing result or temporary probe");
    }

    const auto runtime = configure_deterministic_runtime();
    const torch::Device device(torch::kCUDA, 0);
    const auto development_sha256 = sha256_file(options.input);
    const auto oracle_archive_sha256 = sha256_file(options.oracle_archive);
    const auto development = read_probe(options.input, 0, kDevelopmentEnd);
    if (development.record_schema != kProbeRecordSchema) {
      throw std::runtime_error("development probe record schema is not frozen");
    }

    auto loaded_sidecar =
        mdn::load_per_edge_affine_return_head(options.oracle_archive);
    validate_sidecar_binding(loaded_sidecar, development, development_sha256);
    if (!sidecar_parameters_frozen(loaded_sidecar.head)) {
      throw std::runtime_error("loaded sidecar parameters are not frozen");
    }

    const auto selection_fit_indices = integer_range(0, kSelectionFitEnd);
    const auto selection_validation_indices =
        integer_range(kPurgeEnd, kDevelopmentEnd);
    const auto refit_indices = integer_range(0, kDevelopmentEnd);
    const auto selection_oracle =
        fit_analytic_ridge(development, selection_fit_indices, 1.0e-10);
    const auto oracle_prediction =
        predict_state_from_context(selection_oracle.state, development.context);
    const auto oracle_fit_prediction = oracle_prediction.index_select(
        0, index_tensor(selection_fit_indices, torch::kCPU));
    const auto oracle_fit_target = development.target.index_select(
        0, index_tensor(selection_fit_indices, torch::kCPU));
    const auto oracle_validation_prediction = oracle_prediction.index_select(
        0, index_tensor(selection_validation_indices, torch::kCPU));
    const auto oracle_validation_target = development.target.index_select(
        0, index_tensor(selection_validation_indices, torch::kCPU));
    const auto oracle_fit_metric =
        evaluate_prediction(oracle_fit_prediction, oracle_fit_target);
    const auto oracle_validation_metric =
        evaluate_prediction(oracle_validation_prediction,
                            oracle_validation_target);
    require_metric_counts(oracle_fit_metric, 4986, 4986, "oracle fit");
    require_metric_counts(oracle_validation_metric, 1314, 1314,
                          "oracle validation");
    const bool oracle_pinned_one_thread_fixture_match =
        std::fabs(metric_direction(oracle_validation_metric) -
                  kExpectedOracleDirection) <= 2.0e-12 &&
        std::fabs(metric_rank(oracle_validation_metric) - kExpectedOracleRank) <=
            2.0e-12 &&
        std::fabs(metric_rmse(oracle_validation_metric) -
                  kExpectedPinnedOneThreadOracleRmse) <= 2.0e-12;
    const double legacy_oracle_rmse_drift =
        metric_rmse(oracle_validation_metric) - kLegacyCanonicalOracleRmse;
    const bool legacy_oracle_rmse_drift_pass =
        std::fabs(legacy_oracle_rmse_drift) <= kLegacyOracleRmseDriftBound;
    if (!oracle_pinned_one_thread_fixture_match ||
        !legacy_oracle_rmse_drift_pass) {
      std::ostringstream details;
      details.imbue(std::locale::classic());
      details << std::scientific << std::setprecision(17)
              << "deterministic one-thread selection oracle mismatch: direction="
              << metric_direction(oracle_validation_metric)
              << " rank=" << metric_rank(oracle_validation_metric)
              << " rmse=" << metric_rmse(oracle_validation_metric)
              << " coefficient_l2_norm="
              << selection_oracle.coefficient_l2_norm
              << " maximum_normalized_residual="
              << selection_oracle.maximum_normalized_residual;
      throw std::runtime_error(details.str());
    }

    const auto sidecar_source_state = loaded_sidecar.head->affine_state();
    const Normalization sidecar_normalization{
        sidecar_source_state.feature_mean.to(torch::kFloat32).contiguous(),
        sidecar_source_state.feature_inv_std.to(torch::kFloat32).contiguous()};
    auto sidecar_copy = std::make_shared<TrainableAffineImpl>(
        sidecar_normalization, std::nullopt);
    {
      torch::NoGradGuard no_grad;
      sidecar_copy->weights.copy_(
          sidecar_source_state.weights.to(torch::kFloat32));
      sidecar_copy->bias.copy_(sidecar_source_state.bias.to(torch::kFloat32));
    }
    sidecar_copy->weights.set_requires_grad(false);
    sidecar_copy->bias.set_requires_grad(false);
    sidecar_copy->to(device, torch::kFloat32);
    loaded_sidecar.head->eval();
    sidecar_copy->eval();
    loaded_sidecar.head->validate_registered_state();
    const ModelState sidecar_float32_state{
        sidecar_source_state.feature_mean.to(torch::kFloat32),
        sidecar_source_state.feature_inv_std.to(torch::kFloat32),
        sidecar_source_state.weights.to(torch::kFloat32),
        sidecar_source_state.bias.to(torch::kFloat32)};
    const auto sidecar_copy_state = sidecar_copy->mapped_state();
    const bool sidecar_state_copy_exact =
        model_state_equal(sidecar_float32_state, sidecar_copy_state);
    const bool sidecar_copy_parameters_frozen =
        trainable_affine_parameters_frozen(sidecar_copy);
    if (!sidecar_state_copy_exact || !sidecar_copy_parameters_frozen) {
      throw std::runtime_error(
          std::string("CUDA float32 sidecar control failed: state_copy_exact=") +
          bool_text(sidecar_state_copy_exact) + " copy_parameters_frozen=" +
          bool_text(sidecar_copy_parameters_frozen));
    }

    const auto development_features_device =
        development.dynamic_features.to(device, torch::kFloat32);
    const auto development_target_device =
        development.target.to(device, torch::kFloat32);
    torch::Tensor sidecar_development_context_prediction;
    torch::Tensor sidecar_development_readout_prediction;
    torch::Tensor sidecar_copy_development_prediction;
    {
      torch::NoGradGuard no_grad;
      sidecar_development_context_prediction =
          loaded_sidecar.head->forward(development.context);
      sidecar_development_readout_prediction =
          loaded_sidecar.head->forward_readout_features(
              development.readout_features);
      sidecar_copy_development_prediction =
          sidecar_copy->forward(development_features_device);
    }
    const auto sidecar_context_readout_comparison = compare_predictions(
        sidecar_development_context_prediction,
        sidecar_development_readout_prediction);
    const auto sidecar_copy_comparison = compare_predictions(
        sidecar_development_context_prediction,
        sidecar_copy_development_prediction);
    const auto sidecar_readout_copy_comparison = compare_predictions(
        sidecar_development_readout_prediction,
        sidecar_copy_development_prediction);
    if (!sidecar_context_readout_comparison.torch_equal) {
      std::ostringstream details;
      details.imbue(std::locale::classic());
      details << std::scientific << std::setprecision(17)
              << "CPU sidecar context/readout parity failed: maximum_abs_delta="
              << sidecar_context_readout_comparison.maximum_abs_delta
              << " mean_abs_delta="
              << sidecar_context_readout_comparison.mean_abs_delta
              << " descriptive_context_to_cuda_copy_max_abs_delta="
              << sidecar_copy_comparison.maximum_abs_delta
              << " descriptive_context_to_cuda_copy_mean_abs_delta="
              << sidecar_copy_comparison.mean_abs_delta;
      throw std::runtime_error(details.str());
    }
    const auto sidecar_development_metric = evaluate_prediction(
        sidecar_development_context_prediction, development_target_device);
    const auto sidecar_copy_development_metric = evaluate_prediction(
        sidecar_copy_development_prediction, development_target_device);
    require_metric_counts(sidecar_development_metric, 6570, 6570,
                          "sidecar development");
    require_metric_counts(sidecar_copy_development_metric, 6570, 6570,
                          "CUDA sidecar-copy development");

    const auto selection_schedule =
        make_batch_schedule(kSelectionFitEnd, kSteps);
    const auto refit_schedule = make_batch_schedule(kDevelopmentEnd, kSteps);
    const auto fit_normalization = compute_normalization(
        development.dynamic_features, selection_fit_indices);
    const auto gram_diagnostics = make_gram_diagnostics(
        development, selection_fit_indices, fit_normalization);
    if (std::any_of(gram_diagnostics.begin(), gram_diagnostics.end(),
                    [](const GramDiagnostic &value) {
                      return !value.psd_pass || value.retained_rank <= 0;
                    })) {
      throw std::runtime_error("fit-only Gram diagnostics are invalid");
    }

    const auto selection_oracle_candidate_state =
        remap_oracle_to_candidate_normalization(selection_oracle.state,
                                                fit_normalization);
    auto selection_oracle_cuda_copy =
        std::make_shared<TrainableAffineImpl>(fit_normalization, std::nullopt);
    {
      torch::NoGradGuard no_grad;
      selection_oracle_cuda_copy->weights.copy_(
          selection_oracle_candidate_state.weights);
      selection_oracle_cuda_copy->bias.copy_(
          selection_oracle_candidate_state.bias);
    }
    selection_oracle_cuda_copy->weights.set_requires_grad(false);
    selection_oracle_cuda_copy->bias.set_requires_grad(false);
    selection_oracle_cuda_copy->to(device, torch::kFloat32);
    selection_oracle_cuda_copy->eval();
    const auto selection_oracle_cuda_copy_state =
        selection_oracle_cuda_copy->mapped_state();
    const bool selection_oracle_cuda_state_copy_exact = model_state_equal(
        selection_oracle_candidate_state, selection_oracle_cuda_copy_state);
    const bool selection_oracle_cuda_copy_parameters_frozen =
        trainable_affine_parameters_frozen(selection_oracle_cuda_copy);
    if (!selection_oracle_cuda_state_copy_exact ||
        !selection_oracle_cuda_copy_parameters_frozen) {
      throw std::runtime_error(
          std::string("selection-oracle CUDA copy control failed: ") +
          "state_copy_exact=" +
          bool_text(selection_oracle_cuda_state_copy_exact) +
          " copy_parameters_frozen=" +
          bool_text(selection_oracle_cuda_copy_parameters_frozen));
    }
    torch::Tensor selection_oracle_cuda_fit_prediction;
    torch::Tensor selection_oracle_cuda_validation_prediction;
    {
      torch::NoGradGuard no_grad;
      selection_oracle_cuda_fit_prediction = selected_prediction(
          selection_oracle_cuda_copy, development_features_device,
          selection_fit_indices);
      selection_oracle_cuda_validation_prediction = selected_prediction(
          selection_oracle_cuda_copy, development_features_device,
          selection_validation_indices);
    }
    const auto selection_oracle_cuda_fit_metric = evaluate_prediction(
        selection_oracle_cuda_fit_prediction,
        development_target_device.index_select(
            0, index_tensor(selection_fit_indices, device)));
    const auto selection_oracle_cuda_validation_metric = evaluate_prediction(
        selection_oracle_cuda_validation_prediction,
        development_target_device.index_select(
            0, index_tensor(selection_validation_indices, device)));
    require_metric_counts(selection_oracle_cuda_fit_metric, 4986, 4986,
                          "selection-oracle CUDA copy fit");
    require_metric_counts(selection_oracle_cuda_validation_metric, 1314, 1314,
                          "selection-oracle CUDA copy validation");
    const auto selection_oracle_cuda_fit_comparison = compare_predictions(
        oracle_fit_prediction, selection_oracle_cuda_fit_prediction);
    const auto selection_oracle_cuda_validation_comparison = compare_predictions(
        oracle_validation_prediction,
        selection_oracle_cuda_validation_prediction);
    const auto selection_oracle_cuda_gate = validation_gate(
        selection_oracle_cuda_validation_metric, oracle_validation_metric);

    std::vector<TrainResult> ladder;
    ladder.reserve(4);
    reset_deterministic_seed();
    ladder.push_back(train_adam(
        "L0_raw_mse", development, development_features_device,
        development_target_device, selection_fit_indices,
        selection_validation_indices, selection_schedule,
        ObjectiveKind::RawMse, false, false, kClipNorm,
        oracle_validation_metric, device));
    reset_deterministic_seed();
    ladder.push_back(train_adam(
        "L1_scaled_huber", development, development_features_device,
        development_target_device, selection_fit_indices,
        selection_validation_indices, selection_schedule,
        ObjectiveKind::ScaledHuber, false, false, kClipNorm,
        oracle_validation_metric, device));
    reset_deterministic_seed();
    ladder.push_back(train_adam(
        "L2_plus_direction", development, development_features_device,
        development_target_device, selection_fit_indices,
        selection_validation_indices, selection_schedule,
        ObjectiveKind::Direction, false, false, kClipNorm,
        oracle_validation_metric, device));
    reset_deterministic_seed();
    ladder.push_back(train_adam(
        "L3_plus_rank", development, development_features_device,
        development_target_device, selection_fit_indices,
        selection_validation_indices, selection_schedule,
        ObjectiveKind::Rank, false, false, kClipNorm,
        oracle_validation_metric, device));

    const bool recovery_activated = !ladder.front().gate.pass;
    std::optional<TrainResult> recovery_b1;
    std::optional<TrainResult> recovery_b2;
    std::optional<TrainResult> recovery_b3;
    std::optional<TrainResult> recovery_b4;
    if (recovery_activated) {
      if (ladder.front().clip_count > 0) {
        reset_deterministic_seed();
        recovery_b1 = train_adam(
            "B1_no_clip_if_applicable", development,
            development_features_device, development_target_device,
            selection_fit_indices, selection_validation_indices,
            selection_schedule, ObjectiveKind::RawMse, false, false, 0.0,
            oracle_validation_metric, device);
      }
      reset_deterministic_seed();
      recovery_b2 = train_adam(
          "B2_full_batch_adam", development, development_features_device,
          development_target_device, selection_fit_indices,
          selection_validation_indices, selection_schedule,
          ObjectiveKind::RawMse, true, false, kClipNorm,
          oracle_validation_metric, device);
      reset_deterministic_seed();
      recovery_b3 = train_adam(
          "B3_whitened_minibatch_adam", development,
          development_features_device, development_target_device,
          selection_fit_indices, selection_validation_indices,
          selection_schedule, ObjectiveKind::RawMse, false, true, kClipNorm,
          oracle_validation_metric, device);
      reset_deterministic_seed();
      recovery_b4 = train_lbfgs(
          "B4_full_batch_lbfgs", development, development_features_device,
          development_target_device, selection_fit_indices,
          selection_validation_indices, oracle_validation_metric, device);
    }

    const auto shared_selection_zero_state_digest =
        ladder.front().zero_state_digest;
    const auto require_selection_result =
        [&](const TrainResult &result, const char *surface) {
          if (result.zero_state_digest != shared_selection_zero_state_digest) {
            throw std::runtime_error(
                std::string("selection zero-state digest drift: ") + surface);
          }
          require_metric_counts(result.fit_metric, 4986, 4986, surface);
          require_metric_counts(result.validation_metric, 1314, 1314,
                                surface);
        };
    for (const auto &result : ladder) {
      require_selection_result(result, result.name.c_str());
    }
    if (recovery_b1.has_value()) {
      require_selection_result(*recovery_b1, recovery_b1->name.c_str());
    }
    if (recovery_b2.has_value()) {
      require_selection_result(*recovery_b2, recovery_b2->name.c_str());
      require_selection_result(*recovery_b3, recovery_b3->name.c_str());
      require_selection_result(*recovery_b4, recovery_b4->name.c_str());
    }

    std::vector<std::pair<std::string, const TrainResult *>> candidates;
    candidates.emplace_back(ladder.front().name, &ladder.front());
    if (recovery_b1.has_value()) {
      candidates.emplace_back(recovery_b1->name, &*recovery_b1);
    }
    if (recovery_b2.has_value()) {
      candidates.emplace_back(recovery_b2->name, &*recovery_b2);
    }
    if (recovery_b3.has_value()) {
      candidates.emplace_back(recovery_b3->name, &*recovery_b3);
    }
    if (recovery_b4.has_value()) {
      candidates.emplace_back(recovery_b4->name, &*recovery_b4);
    }
    const TrainResult *selected = nullptr;
    int64_t passing_candidate_count = 0;
    for (const auto &[name, result] : candidates) {
      (void)name;
      if (result->gate.pass) {
        ++passing_candidate_count;
        if (selected == nullptr) {
          selected = result;
        }
      }
    }

    std::optional<TrainResult> refit;
    std::optional<Dataset> historical;
    std::string historical_sha256;
    Metric refit_historical_metric;
    Metric sidecar_historical_metric;
    Metric sidecar_copy_historical_metric;
    PredictionComparison historical_refit_sidecar_comparison;
    PredictionComparison historical_sidecar_context_readout_comparison;
    PredictionComparison historical_sidecar_copy_comparison;
    PredictionComparison historical_sidecar_readout_copy_comparison;
    std::string refit_historical_prediction_digest;
    std::string sidecar_historical_prediction_digest;
    if (selected != nullptr) {
      refit = refit_selected_candidate(
          *selected, development, development_features_device,
          development_target_device, refit_indices,
          selection_validation_indices, refit_schedule,
          oracle_validation_metric, device);
      require_metric_counts(refit->fit_metric, 6570, 6570, "selected refit");
      require_metric_counts(refit->validation_metric, 1314, 1314,
                            "selected refit descriptive validation");

      // The consumed historical diagnostic is intentionally not opened until
      // the first passing validation candidate has been selected and refit.
      historical_sha256 = sha256_file(options.evaluation_input);
      historical.emplace(read_probe(options.evaluation_input, kHistoricalBegin,
                                    kHistoricalEnd));
      validate_historical_identity(development, *historical);
      const auto historical_features_device =
          historical->dynamic_features.to(device, torch::kFloat32);
      const auto historical_target_device =
          historical->target.to(device, torch::kFloat32);
      torch::Tensor refit_prediction;
      torch::Tensor sidecar_prediction;
      torch::Tensor sidecar_readout_prediction;
      torch::Tensor sidecar_copy_prediction;
      {
        torch::NoGradGuard no_grad;
        refit_prediction = refit->head->forward(historical_features_device);
        sidecar_prediction =
            loaded_sidecar.head->forward(historical->context);
        sidecar_readout_prediction =
            loaded_sidecar.head->forward_readout_features(
                historical->readout_features);
        sidecar_copy_prediction =
            sidecar_copy->forward(historical_features_device);
      }
      refit_historical_metric =
          evaluate_prediction(refit_prediction, historical_target_device);
      sidecar_historical_metric =
          evaluate_prediction(sidecar_prediction, historical_target_device);
      sidecar_copy_historical_metric = evaluate_prediction(
          sidecar_copy_prediction, historical_target_device);
      require_metric_counts(refit_historical_metric, 2952, 2952,
                            "historical refit");
      require_metric_counts(sidecar_historical_metric, 2952, 2952,
                            "historical sidecar");
      require_metric_counts(sidecar_copy_historical_metric, 2952, 2952,
                            "historical CUDA sidecar copy");
      historical_refit_sidecar_comparison =
          compare_predictions(refit_prediction, sidecar_prediction);
      historical_sidecar_context_readout_comparison =
          compare_predictions(sidecar_prediction, sidecar_readout_prediction);
      historical_sidecar_copy_comparison =
          compare_predictions(sidecar_prediction, sidecar_copy_prediction);
      historical_sidecar_readout_copy_comparison = compare_predictions(
          sidecar_readout_prediction, sidecar_copy_prediction);
      if (!historical_sidecar_context_readout_comparison.torch_equal) {
        std::ostringstream details;
        details.imbue(std::locale::classic());
        details << std::scientific << std::setprecision(17)
                << "historical CPU sidecar context/readout parity failed: "
                   "maximum_abs_delta="
                << historical_sidecar_context_readout_comparison
                       .maximum_abs_delta
                << " mean_abs_delta="
                << historical_sidecar_context_readout_comparison.mean_abs_delta;
        throw std::runtime_error(details.str());
      }
      refit_historical_prediction_digest = prediction_digest(refit_prediction);
      sidecar_historical_prediction_digest =
          prediction_digest(sidecar_prediction);
    }
    require_cuda_success(cudaDeviceSynchronize(), "cudaDeviceSynchronize");

    std::ostringstream probe;
    probe.imbue(std::locale::classic());
    probe << std::scientific << std::setprecision(17);
    probe << "schema_id=" << options.schema_id << '\n';
    probe << "status=complete\n";
    probe << "runtime.torch_version=" << TORCH_VERSION << '\n';
    probe << "runtime.cuda_compile_version=" << CUDART_VERSION << '\n';
    probe << "runtime.cuda_runtime_version=" << runtime.cuda_runtime_version
          << '\n';
    probe << "runtime.cuda_driver_version=" << runtime.cuda_driver_version
          << '\n';
    probe << "runtime.cuda_device_count=" << runtime.cuda_device_count << '\n';
    probe << "runtime.cuda_device_index=0\n";
    probe << "runtime.cuda_device_name=" << runtime.cuda_device_name << '\n';
    probe << "runtime.cuda_compute_capability=" << runtime.cuda_compute_major
          << '.' << runtime.cuda_compute_minor << '\n';
    probe << "runtime.cuda_total_global_memory_bytes="
          << runtime.cuda_total_global_memory << '\n';
    probe << "runtime.cpu_thread_count=1\n";
    probe << "runtime.cpu_interop_thread_count=1\n";
    probe << "runtime.deterministic_algorithms="
          << bool_text(at::globalContext().deterministicAlgorithms()) << '\n';
    probe << "runtime.deterministic_warn_only="
          << bool_text(at::globalContext().deterministicAlgorithmsWarnOnly())
          << '\n';
    probe << "runtime.deterministic_cudnn="
          << bool_text(at::globalContext().deterministicCuDNN()) << '\n';
    probe << "runtime.cudnn_benchmark="
          << bool_text(at::globalContext().benchmarkCuDNN()) << '\n';
    probe << "runtime.allow_tf32_cublas="
          << bool_text(at::globalContext().allowTF32CuBLAS()) << '\n';
    probe << "runtime.allow_tf32_cudnn="
          << bool_text(at::globalContext().allowTF32CuDNN()) << '\n';
    probe << "runtime.cublas_workspace_config=:4096:8\n";
    probe << "runtime.cuda_device_order=PCI_BUS_ID\n";
    probe << "runtime.seed=" << kSeed << '\n';
    probe << "input.development.sha256=" << development_sha256 << '\n';
    probe << "input.oracle_archive.sha256=" << oracle_archive_sha256 << '\n';
    probe << "input.historical.opened=" << bool_text(historical.has_value())
          << '\n';
    if (historical.has_value()) {
      probe << "input.historical.sha256=" << historical_sha256 << '\n';
    }
    probe << "data.record_schema=" << development.record_schema << '\n';
    probe << "data.development.row_count=" << development.row_count << '\n';
    probe << "data.development.anchor_count="
          << development.dynamic_features.size(0) << '\n';
    probe << "data.feature_dim=" << kFeatureDim << '\n';
    probe << "data.dynamic_feature_count=" << kDynamicFeatureCount << '\n';
    probe << "data.source_feature_count=" << kSourceFeatureCount << '\n';
    probe << "data.edge_count=" << kEdgeCount << '\n';
    probe << "data.channel_count=" << kChannelCount << '\n';
    probe << "data.quote_node_id=" << development.quote_node_id << '\n';
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      probe << "data.edge." << edge << ".edge_id="
            << development.edge_ids[static_cast<std::size_t>(edge)] << '\n';
      probe << "data.edge." << edge << ".base_node_id="
            << development.base_node_ids[static_cast<std::size_t>(edge)]
            << '\n';
    }
    probe << "data.cached_prefix_torch_equal="
          << bool_text(development.prefix_torch_equal) << '\n';
    probe << "data.cached_prefix_max_abs_delta="
          << development.prefix_max_abs_delta << '\n';
    probe << "split.selection_fit=[0,554)\n";
    probe << "split.selection_purge=[554,584)\n";
    probe << "split.selection_validation=[584,730)\n";
    probe << "split.refit=[0,730)\n";
    probe << "split.outer_purge=[730,760)\n";
    probe << "split.historical_confirmation=[760,1088)\n";
    probe << "split.unconsumed_holdout=[1088,1170)\n";
    probe << "data.unconsumed_holdout_opened=false\n";
    probe << "data.maximum_anchor_loaded="
          << (historical.has_value() ? kHistoricalEnd - 1
                                     : kDevelopmentEnd - 1)
          << '\n';

    probe << "execution.initialization=all_affine_weights_and_biases_zero\n";
    probe << "execution.selection_zero_state_digest="
          << shared_selection_zero_state_digest << '\n';
    probe << "execution.adam.learning_rate=0.001\n";
    probe << "execution.adam.beta1=0.9\n";
    probe << "execution.adam.beta2=0.999\n";
    probe << "execution.adam.epsilon=1e-8\n";
    probe << "execution.adam.weight_decay=0\n";
    probe << "execution.adam.amsgrad=false\n";
    probe << "execution.adam.steps=3500\n";
    probe << "execution.adam.minibatch_size=64\n";
    probe << "execution.adam.production_clip_norm=5.0\n";
    probe << "execution.schedule.kind=production_random_sampler_epoch_reseeded\n";
    probe << "execution.schedule.seed=2031\n";
    probe << "execution.objective.L0=raw_mean_square_error\n";
    probe << "execution.objective.L0.target_scale=1\n";
    probe << "execution.objective.L0.regression_weight=1\n";
    probe << "execution.objective.L1=smooth_l1\n";
    probe << "execution.objective.target_scale=36\n";
    probe << "execution.objective.smooth_l1_beta=0.5\n";
    probe << "execution.objective.regression_weight=100\n";
    probe << "execution.objective.direction_weight=5\n";
    probe << "execution.objective.rank_weight=5\n";
    probe << "execution.objective.logit_scale=1\n";
    probe << "execution.objective.rank_pairs=all_unordered_edges_per_anchor_channel\n";
    probe << "execution.gate.policy=one_sided_noninferiority\n";
    probe << "execution.gate.direction_shortfall_max=0.01\n";
    probe << "execution.gate.rank_shortfall_max=0.01\n";
    probe << "execution.gate.rmse_excess_max=0.0005\n";
    probe << "execution.pca.scope=selection_fit_only_per_edge_after_shared_standardization\n";
    probe << "execution.pca.retention_threshold=max_1e-12_or_1e-10_times_lambda_max\n";
    probe << "execution.pca.cuda_float32_covariance_error_max=0.05\n";
    probe << "execution.pca.mapped_prediction_max_abs_delta=0.00002\n";
    probe << "execution.pca.mapped_prediction_policy=selection_deployability_gate_nonfatal_to_harness\n";
    probe << "execution.lbfgs.learning_rate=1.0\n";
    probe << "execution.lbfgs.line_search=strong_wolfe\n";
    probe << "execution.lbfgs.max_iterations=500\n";
    probe << "execution.lbfgs.max_evaluations=625\n";
    probe << "execution.lbfgs.history_size=100\n";
    probe << "execution.lbfgs.tolerance_grad=1e-12\n";
    probe << "execution.lbfgs.tolerance_change=1e-15\n";

    probe << "oracle.kind=analytic_ridge_alpha_1e-10\n";
    probe << "oracle.feature_surface=context_float32_promoted_to_float64_then_base_quote_difference_recomputed\n";
    probe << "oracle.thread_policy=deterministic_at_num_threads_1\n";
    probe << "oracle.gate_reference=pinned_one_thread_observed_oracle\n";
    probe << "oracle.fit_scope=[0,554)\n";
    probe << "oracle.validation_scope=[584,730)\n";
    probe << "oracle.pinned_one_thread_fixture_match="
          << bool_text(oracle_pinned_one_thread_fixture_match) << '\n';
    probe << "oracle.legacy_reference_exactly_reproduced=false\n";
    probe << "oracle.legacy_reference.validation.rmse="
          << kLegacyCanonicalOracleRmse << '\n';
    probe << "oracle.legacy_reference.validation.directional_accuracy="
          << kExpectedOracleDirection << '\n';
    probe << "oracle.legacy_reference.validation.pairwise_rank_accuracy="
          << kExpectedOracleRank << '\n';
    probe << "oracle.legacy_reference.rmse_drift="
          << legacy_oracle_rmse_drift << '\n';
    probe << "oracle.legacy_reference.rmse_abs_drift="
          << std::fabs(legacy_oracle_rmse_drift) << '\n';
    probe << "oracle.legacy_reference.rmse_abs_drift_max="
          << kLegacyOracleRmseDriftBound << '\n';
    probe << "oracle.legacy_reference.rmse_drift_pass="
          << bool_text(legacy_oracle_rmse_drift_pass) << '\n';
    probe << "oracle.maximum_normalized_residual="
          << selection_oracle.maximum_normalized_residual << '\n';
    probe << "oracle.coefficient_l2_norm="
          << selection_oracle.coefficient_l2_norm << '\n';
    probe << "oracle.state_digest=" << state_digest(selection_oracle.state)
          << '\n';
    probe << "oracle.validation_prediction_digest="
          << prediction_digest(oracle_validation_prediction) << '\n';
    emit_metric(probe, "oracle.fit", oracle_fit_metric);
    emit_metric(probe, "oracle.validation", oracle_validation_metric);
    probe << "oracle.expected.pinned_one_thread.validation.rmse="
          << kExpectedPinnedOneThreadOracleRmse << '\n';
    probe << "oracle.expected.pinned_one_thread.validation.directional_accuracy="
          << kExpectedOracleDirection << '\n';
    probe << "oracle.expected.pinned_one_thread.validation.pairwise_rank_accuracy="
          << kExpectedOracleRank << '\n';

    probe << "control.P0_sidecar_copy.applicable=true\n";
    probe << "control.P0_sidecar_copy.selection_eligible=false\n";
    probe << "control.P0_sidecar_copy.role=development_representability_not_selection\n";
    probe << "control.P0_sidecar_copy.semantic_tensor_digest="
          << loaded_sidecar.metadata.semantic_tensor_digest << '\n';
    probe << "control.P0_sidecar_copy.parameters_frozen="
          << bool_text(sidecar_parameters_frozen(loaded_sidecar.head)) << '\n';
    probe << "control.P0_sidecar_copy.state_copy_exact="
          << bool_text(sidecar_state_copy_exact) << '\n';
    probe << "control.P0_sidecar_copy.copy_parameters_frozen="
          << bool_text(sidecar_copy_parameters_frozen) << '\n';
    probe << "control.P0_sidecar_copy.baseline_device=cpu\n";
    probe << "control.P0_sidecar_copy.baseline_dtype=float64_state_float32_output\n";
    probe << "control.P0_sidecar_copy.copy_device=cuda_0\n";
    probe << "control.P0_sidecar_copy.copy_dtype=float32\n";
    probe << "control.P0_sidecar_copy.cross_dtype_device_delta_policy=descriptive_no_numeric_acceptance_gate\n";
    probe << "control.P0_sidecar_copy.cross_dtype_device_delta_semantics=cpu_float64_accumulation_vs_cuda_float32_accumulation\n";
    probe << "control.P0_sidecar_copy.state_digest="
          << state_digest(sidecar_float32_state) << '\n';
    probe << "control.P0_sidecar_copy.copy_state_digest="
          << state_digest(sidecar_copy_state) << '\n';
    probe << "control.P0_sidecar_copy.development_prediction_digest="
          << prediction_digest(sidecar_development_context_prediction) << '\n';
    emit_comparison(probe,
                    "control.P0_sidecar_copy.development_context_to_readout",
                    sidecar_context_readout_comparison);
    emit_comparison(probe,
                    "control.P0_sidecar_copy.development_sidecar_to_copy",
                    sidecar_copy_comparison);
    emit_comparison(
        probe,
        "control.P0_sidecar_copy.development_readout_sidecar_to_copy",
        sidecar_readout_copy_comparison);
    emit_metric(probe, "control.P0_sidecar_copy.development",
                sidecar_development_metric);
    emit_metric(probe, "control.P0_sidecar_copy.copy_development",
                sidecar_copy_development_metric);

    probe << "control.P1_selection_oracle_cuda_copy.applicable=true\n";
    probe << "control.P1_selection_oracle_cuda_copy.selection_eligible=false\n";
    probe << "control.P1_selection_oracle_cuda_copy.role=selection_surface_representability_control\n";
    probe << "control.P1_selection_oracle_cuda_copy.fit_scope=[0,554)\n";
    probe << "control.P1_selection_oracle_cuda_copy.validation_scope=[584,730)\n";
    probe << "control.P1_selection_oracle_cuda_copy.historical_evaluated=false\n";
    probe << "control.P1_selection_oracle_cuda_copy.candidate_normalization=cached_float32_selection_fit_only\n";
    probe << "control.P1_selection_oracle_cuda_copy.remap=raw_slope_oracle_inv_std_times_weight_then_candidate_weight_raw_slope_div_candidate_inv_std_with_intercept_correction\n";
    probe << "control.P1_selection_oracle_cuda_copy.cross_surface_delta_policy=descriptive_nonfatal\n";
    probe << "control.P1_selection_oracle_cuda_copy.source_device=cpu\n";
    probe << "control.P1_selection_oracle_cuda_copy.source_dtype=float64\n";
    probe << "control.P1_selection_oracle_cuda_copy.copy_device=cuda_0\n";
    probe << "control.P1_selection_oracle_cuda_copy.copy_dtype=float32\n";
    probe << "control.P1_selection_oracle_cuda_copy.state_copy_exact="
          << bool_text(selection_oracle_cuda_state_copy_exact) << '\n';
    probe << "control.P1_selection_oracle_cuda_copy.copy_parameters_frozen="
          << bool_text(selection_oracle_cuda_copy_parameters_frozen) << '\n';
    probe << "control.P1_selection_oracle_cuda_copy.state_digest="
          << state_digest(selection_oracle_cuda_copy_state) << '\n';
    probe << "control.P1_selection_oracle_cuda_copy.fit_prediction_digest="
          << prediction_digest(selection_oracle_cuda_fit_prediction) << '\n';
    probe << "control.P1_selection_oracle_cuda_copy.validation_prediction_digest="
          << prediction_digest(selection_oracle_cuda_validation_prediction)
          << '\n';
    emit_comparison(
        probe, "control.P1_selection_oracle_cuda_copy.fit_oracle_to_copy",
        selection_oracle_cuda_fit_comparison);
    emit_comparison(
        probe,
        "control.P1_selection_oracle_cuda_copy.validation_oracle_to_copy",
        selection_oracle_cuda_validation_comparison);
    emit_metric(probe, "control.P1_selection_oracle_cuda_copy.fit",
                selection_oracle_cuda_fit_metric);
    emit_metric(probe, "control.P1_selection_oracle_cuda_copy.validation",
                selection_oracle_cuda_validation_metric);
    probe << "control.P1_selection_oracle_cuda_copy.gate.direction_shortfall="
          << selection_oracle_cuda_gate.direction_shortfall << '\n';
    probe << "control.P1_selection_oracle_cuda_copy.gate.rank_shortfall="
          << selection_oracle_cuda_gate.rank_shortfall << '\n';
    probe << "control.P1_selection_oracle_cuda_copy.gate.rmse_excess="
          << selection_oracle_cuda_gate.rmse_excess << '\n';
    probe << "control.P1_selection_oracle_cuda_copy.validation_parity_pass="
          << bool_text(selection_oracle_cuda_gate.pass) << '\n';

    emit_schedule(probe, "schedule.selection", selection_schedule);
    emit_gram_diagnostics(probe, gram_diagnostics);
    probe << "objective_ladder.arm_order=L0_raw_mse,L1_scaled_huber,L2_plus_direction,L3_plus_rank\n";
    probe << "objective_ladder.shared_zero_initialization=true\n";
    probe << "objective_ladder.shared_schedule=true\n";
    probe << "objective_ladder.adjacent_delta_interpretation="
          << (recovery_activated ? "descriptive_noncausal_l0_failed"
                                 : "descriptive_l0_passed")
          << '\n';
    emit_arm(probe, "arm.L0_raw_mse", ladder[0], true);
    emit_arm(probe, "arm.L1_scaled_huber", ladder[1], false);
    emit_arm(probe, "arm.L2_plus_direction", ladder[2], false);
    emit_arm(probe, "arm.L3_plus_rank", ladder[3], false);
    emit_adjacent_delta(probe, "objective_delta.L0_to_L1.validation",
                        ladder[0].validation_metric,
                        ladder[1].validation_metric);
    emit_adjacent_delta(probe, "objective_delta.L1_to_L2.validation",
                        ladder[1].validation_metric,
                        ladder[2].validation_metric);
    emit_adjacent_delta(probe, "objective_delta.L2_to_L3.validation",
                        ladder[2].validation_metric,
                        ladder[3].validation_metric);

    probe << "recovery.activated=" << bool_text(recovery_activated) << '\n';
    probe << "recovery.trigger="
          << (recovery_activated ? "l0_validation_parity_failure"
                                 : "none_l0_validation_parity_passed")
          << '\n';
    probe << "recovery.order=B1_no_clip_if_applicable,B2_full_batch_adam,B3_whitened_minibatch_adam,B4_full_batch_lbfgs\n";
    if (recovery_b1.has_value()) {
      emit_arm(probe, "recovery.B1_no_clip_if_applicable", *recovery_b1,
               true);
    } else {
      emit_skipped_arm(
          probe, "recovery.B1_no_clip_if_applicable",
          recovery_activated ? "l0_gradient_clip_count_zero"
                             : "recovery_not_activated_l0_passed");
    }
    if (recovery_b2.has_value()) {
      emit_arm(probe, "recovery.B2_full_batch_adam", *recovery_b2, true);
      emit_arm(probe, "recovery.B3_whitened_minibatch_adam", *recovery_b3,
               true);
      emit_arm(probe, "recovery.B4_full_batch_lbfgs", *recovery_b4, true);
    } else {
      emit_skipped_arm(probe, "recovery.B2_full_batch_adam",
                       "recovery_not_activated_l0_passed");
      emit_skipped_arm(probe, "recovery.B3_whitened_minibatch_adam",
                       "recovery_not_activated_l0_passed");
      emit_skipped_arm(probe, "recovery.B4_full_batch_lbfgs",
                       "recovery_not_activated_l0_passed");
    }

    probe << "selection.policy=first_passing_candidate_in_predeclared_order\n";
    probe << "selection.candidate_order=L0_raw_mse,B1_no_clip_if_applicable,B2_full_batch_adam,B3_whitened_minibatch_adam,B4_full_batch_lbfgs\n";
    probe << "selection.historical_used=false\n";
    probe << "selection.passing_candidate_count=" << passing_candidate_count
          << '\n';
    probe << "selection.found=" << bool_text(selected != nullptr) << '\n';
    probe << "selection.selected="
          << (selected != nullptr ? selected->name : "none") << '\n';
    if (selected != nullptr) {
      probe << "selection.selected_validation_prediction_digest="
            << selected->validation_prediction_digest << '\n';
      probe << "refit.applicable=true\n";
      probe << "refit.source_candidate=" << selected->name << '\n';
      probe << "refit.selection_eligible=false\n";
      probe << "refit.statistics_scope=[0,730)\n";
      probe << "refit.validation_metrics_role=descriptive_post_selection\n";
      emit_schedule(probe, "schedule.refit", refit_schedule);
      emit_arm(probe, "refit.selected", *refit, false);
      probe << "historical.evaluated=true\n";
      probe << "historical.role=previously_consumed_diagnostic_confirmation\n";
      probe << "historical.used_for_selection=false\n";
      probe << "historical.row_count=" << historical->row_count << '\n';
      probe << "historical.anchor_count="
            << historical->dynamic_features.size(0) << '\n';
      probe << "historical.refit_prediction_digest="
            << refit_historical_prediction_digest << '\n';
      probe << "historical.sidecar_prediction_digest="
            << sidecar_historical_prediction_digest << '\n';
      emit_metric(probe, "historical.refit", refit_historical_metric);
      emit_metric(probe, "historical.sidecar", sidecar_historical_metric);
      emit_metric(probe, "historical.sidecar_copy",
                  sidecar_copy_historical_metric);
      emit_comparison(probe, "historical.refit_to_sidecar",
                      historical_refit_sidecar_comparison);
      emit_comparison(probe, "historical.sidecar_context_to_readout",
                      historical_sidecar_context_readout_comparison);
      emit_comparison(probe, "historical.sidecar_to_copy",
                      historical_sidecar_copy_comparison);
      emit_comparison(probe, "historical.sidecar_readout_to_copy",
                      historical_sidecar_readout_copy_comparison);
    } else {
      probe << "refit.applicable=false\n";
      probe << "refit.reason=no_passing_validation_candidate\n";
      probe << "historical.evaluated=false\n";
    }

    write_probe_atomic(options.output, probe.str());
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "mdn_frozen_affine_objective_ladder: " << error.what()
              << '\n';
    return 1;
  }
}
