#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <map>
#include <numeric>
#include <sstream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <ATen/Context.h>
#include <ATen/Parallel.h>
#include <ATen/ops/cholesky_solve.h>
#include <ATen/ops/linalg_cholesky_ex.h>
#include <torch/torch.h>

namespace {

constexpr std::string_view kProbeHeader =
    "record_schema,anchor_key,anchor_index,anchor_local_index,edge_index,"
    "edge_id,base_node_id,quote_node_id,channel_index,"
    "target_edge_close_return,feature_count,feature_values";
constexpr int64_t kTrainBegin = 0;
constexpr int64_t kTrainEnd = 2496;
constexpr int64_t kValidationBegin = 2560;
constexpr int64_t kValidationEnd = 2816;
constexpr int64_t kCertifiedBegin = 2880;
constexpr int64_t kCertifiedEnd = 3261;
constexpr int64_t kFinalBegin = 3328;
constexpr int64_t kEdgeCount = 3;
constexpr int64_t kChannelCount = 3;
constexpr double kTieTolerance = 1.0e-12;
constexpr std::array<double, 6> kRidgeGrid{
    1.0e-12, 1.0e-10, 1.0e-8, 1.0e-6, 1.0e-4, 1.0e-2};

const std::array<std::string, 3> kEdgeIds{
    "SYN2ALPHASYN2USD", "SYN2BETASYN2USD", "SYN2GAMMASYN2USD"};
const std::array<std::string, 3> kBaseIds{"SYN2ALPHA", "SYN2BETA",
                                                 "SYN2GAMMA"};
constexpr std::string_view kQuoteId = "SYN2USD";

enum class ProbeKind { kMdnContext, kRepresentation };

struct ProbeSpec {
  ProbeKind kind;
  std::string_view name;
  std::string_view report_schema;
  std::string_view record_schema;
  int64_t representation_width;
  int64_t affine_width;
  int64_t probe_width;
};

constexpr ProbeSpec kMdnContextSpec{
    .kind = ProbeKind::kMdnContext,
    .name = "mdn_context",
    .report_schema = "synthetic_v2_frozen_representation_affine_probe_v1",
    .record_schema =
        "kikijyeba.synthetic.mdn_edge_context_feature_probe.v1",
    .representation_width = 128,
    .affine_width = 384,
    .probe_width = 400};

constexpr ProbeSpec kRepresentationSpec{
    .kind = ProbeKind::kRepresentation,
    .name = "representation",
    .report_schema = "synthetic_v2_frozen_encoder_affine_probe_v1",
    .record_schema =
        "kikijyeba.synthetic.representation_edge_feature_probe.v1",
    .representation_width = 32,
    .affine_width = 96,
    .probe_width = 96};

struct Options {
  const ProbeSpec *probe_spec{nullptr};
  bool validation_only{false};
  bool development_only{false};
  std::filesystem::path train_input;
  std::filesystem::path validation_input;
  std::filesystem::path certified_input;
  std::filesystem::path selection_lock;
  std::filesystem::path output;
};

struct Dataset {
  torch::Tensor features; // [A,E,C,D], CPU float64.
  torch::Tensor target;   // [A,E,C], CPU float64.
  int64_t anchor_begin{0};
  int64_t anchor_end{0};
  int64_t rows{0};
  double context_identity_max_abs_delta{0.0};
};

struct Metric {
  int64_t count{0};
  int64_t direction_correct{0};
  int64_t pair_count{0};
  int64_t pair_correct{0};
  int64_t best_count{0};
  int64_t best_correct{0};
  double abs_error_sum{0.0};
  double square_error_sum{0.0};
  double prediction_sum{0.0};
  double prediction_square_sum{0.0};
  double target_sum{0.0};
  double target_square_sum{0.0};
  double cross_sum{0.0};
};

struct MetricSummary {
  int64_t count{0};
  int64_t pair_count{0};
  double mae{0.0};
  double rmse{0.0};
  double target_rms{0.0};
  double prediction_rms{0.0};
  double rmse_target_rms_ratio{0.0};
  double direction{0.0};
  double rank{0.0};
  double best_asset{0.0};
  double correlation{0.0};
};

struct Model {
  torch::Tensor mean;    // [D]
  torch::Tensor inv_std; // [D]
  torch::Tensor weights; // [E,D]
  torch::Tensor bias;    // [E]
  int64_t feature_width{0};
  double ridge{0.0};
  double maximum_normalized_residual{0.0};
  double coefficient_l2_norm{0.0};
};

struct Candidate {
  bool numerically_valid{false};
  double declared_ridge{0.0};
  std::string rejection_reason;
  Model model;
  MetricSummary validation;
};

class CandidateNumericalError : public std::runtime_error {
public:
  explicit CandidateNumericalError(const std::string &message)
      : std::runtime_error(message) {}
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

int64_t parse_i64(const std::string &text, const char *field) {
  if (text.empty()) {
    throw std::runtime_error(std::string("empty integer field: ") + field);
  }
  std::size_t consumed = 0;
  const auto value = std::stoll(text, &consumed, 10);
  if (consumed != text.size()) {
    throw std::runtime_error(std::string("invalid integer field: ") + field);
  }
  return value;
}

double parse_f64(const std::string &text, const char *field) {
  if (text.empty()) {
    throw std::runtime_error(std::string("empty floating field: ") + field);
  }
  std::size_t consumed = 0;
  const auto value = std::stod(text, &consumed);
  if (consumed != text.size() || !std::isfinite(value)) {
    throw std::runtime_error(std::string("invalid floating field: ") + field);
  }
  return value;
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
    if (argument == "--probe-kind") {
      const auto kind = value("--probe-kind");
      if (kind == kMdnContextSpec.name) {
        options.probe_spec = &kMdnContextSpec;
      } else if (kind == kRepresentationSpec.name) {
        options.probe_spec = &kRepresentationSpec;
      } else {
        throw std::runtime_error("invalid --probe-kind: " + kind);
      }
    } else if (argument == "--validation-only") {
      options.validation_only = true;
    } else if (argument == "--development-only") {
      options.development_only = true;
    } else if (argument == "--train-input") {
      options.train_input = value("--train-input");
    } else if (argument == "--validation-input") {
      options.validation_input = value("--validation-input");
    } else if (argument == "--certified-input") {
      options.certified_input = value("--certified-input");
    } else if (argument == "--selection-lock") {
      if (!options.selection_lock.empty()) {
        throw std::runtime_error("duplicate --selection-lock");
      }
      options.selection_lock = value("--selection-lock");
    } else if (argument == "--output") {
      options.output = value("--output");
    } else if (argument == "--evaluation-input" ||
               argument == "--holdout-input" ||
               argument == "--final-input" || argument == "--refit-input" ||
               argument == "--policy-input") {
      throw std::runtime_error("forbidden additional input: " + argument);
    } else {
      throw std::runtime_error("unknown argument: " + argument);
    }
  }
  if (options.validation_only && options.development_only) {
    throw std::runtime_error(
        "--validation-only and --development-only are mutually exclusive");
  }
  const bool omits_certified =
      options.validation_only || options.development_only;
  if (omits_certified && !options.selection_lock.empty()) {
    throw std::runtime_error(
        "--selection-lock is accepted only in full certified mode");
  }
  if (!omits_certified && options.selection_lock.empty()) {
    throw std::runtime_error(
        "--selection-lock is required in full certified mode");
  }
  if (options.probe_spec == nullptr || options.train_input.empty() ||
      options.validation_input.empty() || options.output.empty() ||
      (!omits_certified && options.certified_input.empty()) ||
      (omits_certified && !options.certified_input.empty())) {
    throw std::runtime_error(
        "--probe-kind --train-input --validation-input --certified-input "
        "and --output are required, or use --validation-only or "
        "--development-only without a certified input");
  }
  std::set<std::filesystem::path> paths;
  std::vector<std::filesystem::path> option_paths{
      options.train_input, options.validation_input, options.output};
  if (!options.certified_input.empty()) {
    option_paths.push_back(options.certified_input);
  }
  if (!options.selection_lock.empty()) {
    option_paths.push_back(options.selection_lock);
  }
  for (const auto &path : option_paths) {
    std::error_code error;
    const auto absolute = std::filesystem::absolute(path, error);
    if (error || !paths.insert(absolute.lexically_normal()).second) {
      throw std::runtime_error(
          "all probe inputs, the selection lock, and the output must be "
          "distinct");
    }
  }
  if (!options.selection_lock.empty()) {
    const auto lock_text = options.selection_lock.string();
    if (lock_text.find('\n') != std::string::npos ||
        lock_text.find('\r') != std::string::npos) {
      throw std::runtime_error("selection lock path contains a newline");
    }
  }
  return options;
}

Dataset read_probe(const std::filesystem::path &path, int64_t expected_begin,
                   int64_t expected_end, const ProbeSpec &spec) {
  if (expected_begin < 0 || expected_end <= expected_begin ||
      expected_end > kFinalBegin) {
    throw std::runtime_error("invalid declared probe range");
  }
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open frozen feature probe");
  }
  input.imbue(std::locale::classic());
  std::string line;
  if (!std::getline(input, line) || line != kProbeHeader) {
    throw std::runtime_error("frozen feature probe header mismatch");
  }

  const auto anchor_count = expected_end - expected_begin;
  const auto total_rows = anchor_count * kEdgeCount * kChannelCount;
  const auto nan = std::numeric_limits<double>::quiet_NaN();
  std::vector<double> features(
      static_cast<std::size_t>(total_rows * spec.affine_width), nan);
  std::vector<double> targets(static_cast<std::size_t>(total_rows), nan);
  std::vector<bool> seen(static_cast<std::size_t>(total_rows), false);
  double identity_delta = 0.0;
  int64_t rows = 0;

  const auto row_index = [](int64_t local_anchor, int64_t edge,
                            int64_t channel) {
    return (local_anchor * kEdgeCount + edge) * kChannelCount + channel;
  };

  while (std::getline(input, line)) {
    if (line.empty()) {
      throw std::runtime_error("frozen feature probe has an empty row");
    }
    const auto columns = split_exact(line, ',');
    if (columns.size() != 12 || columns[0] != spec.record_schema) {
      throw std::runtime_error("frozen feature probe row schema mismatch");
    }
    (void)parse_i64(columns[1], "anchor_key");
    const auto anchor = parse_i64(columns[2], "anchor_index");
    const auto batch_local_anchor =
        parse_i64(columns[3], "anchor_local_index");
    const auto edge = parse_i64(columns[4], "edge_index");
    const auto channel = parse_i64(columns[8], "channel_index");
    const auto target = parse_f64(columns[9], "target_edge_close_return");
    const auto feature_count = parse_i64(columns[10], "feature_count");
    if (anchor < expected_begin || anchor >= expected_end ||
        anchor >= kFinalBegin || batch_local_anchor < 0 ||
        edge < 0 || edge >= kEdgeCount ||
        channel < 0 || channel >= kChannelCount ||
        feature_count != spec.probe_width || columns[5] != kEdgeIds[edge] ||
        columns[6] != kBaseIds[edge] || columns[7] != kQuoteId) {
      throw std::runtime_error("frozen feature probe row domain mismatch");
    }
    const auto row = row_index(anchor - expected_begin, edge, channel);
    if (seen[static_cast<std::size_t>(row)]) {
      throw std::runtime_error("duplicate frozen feature probe coordinate");
    }
    seen[static_cast<std::size_t>(row)] = true;
    targets[static_cast<std::size_t>(row)] = target;
    const auto cells = split_exact(columns[11], ';');
    if (static_cast<int64_t>(cells.size()) != spec.probe_width) {
      throw std::runtime_error("frozen feature width mismatch");
    }
    std::vector<float> parsed(static_cast<std::size_t>(spec.affine_width),
                              0.0F);
    for (int64_t feature = 0; feature < spec.probe_width; ++feature) {
      const auto value = parse_f64(cells[static_cast<std::size_t>(feature)],
                                   "feature_value");
      if (feature < spec.affine_width) {
        parsed[static_cast<std::size_t>(feature)] =
            static_cast<float>(value);
        features[static_cast<std::size_t>(row * spec.affine_width + feature)] =
            static_cast<double>(parsed[static_cast<std::size_t>(feature)]);
      }
    }
    for (int64_t feature = 0; feature < spec.representation_width; ++feature) {
      const float reconstructed =
          parsed[static_cast<std::size_t>(feature)] -
          parsed[static_cast<std::size_t>(spec.representation_width + feature)];
      identity_delta = std::max(
          identity_delta,
          std::fabs(static_cast<double>(
              reconstructed -
              parsed[static_cast<std::size_t>(2 * spec.representation_width +
                                               feature)])));
    }
    ++rows;
  }
  if (!input.good() && !input.eof()) {
    throw std::runtime_error("failed while reading frozen feature probe");
  }
  if (rows != total_rows ||
      std::find(seen.begin(), seen.end(), false) != seen.end()) {
    throw std::runtime_error("frozen feature probe is not a complete cube");
  }
  if (!(identity_delta <= 2.0e-6)) {
    throw std::runtime_error("base-minus-quote feature identity failed");
  }

  Dataset dataset;
  dataset.features =
      torch::from_blob(features.data(),
                       {anchor_count, kEdgeCount, kChannelCount,
                        spec.affine_width},
                       torch::kFloat64)
          .clone();
  dataset.target =
      torch::from_blob(targets.data(),
                       {anchor_count, kEdgeCount, kChannelCount},
                       torch::kFloat64)
          .clone();
  if (!torch::isfinite(dataset.features).all().item<bool>() ||
      !torch::isfinite(dataset.target).all().item<bool>()) {
    throw std::runtime_error("frozen feature probe contains nonfinite values");
  }
  dataset.anchor_begin = expected_begin;
  dataset.anchor_end = expected_end;
  dataset.rows = rows;
  dataset.context_identity_max_abs_delta = identity_delta;
  return dataset;
}

Metric observe(const torch::Tensor &prediction_input,
               const torch::Tensor &target_input) {
  const auto prediction =
      prediction_input.to(torch::kCPU, torch::kFloat64).contiguous();
  const auto target = target_input.to(torch::kCPU, torch::kFloat64).contiguous();
  if (prediction.sizes() != target.sizes() || prediction.dim() != 3 ||
      prediction.size(1) != kEdgeCount) {
    throw std::runtime_error("metric tensor shape mismatch");
  }
  const auto p = prediction.accessor<double, 3>();
  const auto t = target.accessor<double, 3>();
  const auto sign = [](double value) {
    return value > 0.0 ? 1 : (value < 0.0 ? -1 : 0);
  };
  Metric metric;
  for (int64_t anchor = 0; anchor < prediction.size(0); ++anchor) {
    for (int64_t channel = 0; channel < prediction.size(2); ++channel) {
      int64_t predicted_best = 0;
      int64_t realized_best = 0;
      for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
        const double predicted = p[anchor][edge][channel];
        const double realized = t[anchor][edge][channel];
        if (!std::isfinite(predicted) || !std::isfinite(realized)) {
          throw std::runtime_error("nonfinite metric value");
        }
        ++metric.count;
        const double error = predicted - realized;
        metric.abs_error_sum += std::fabs(error);
        metric.square_error_sum += error * error;
        metric.prediction_sum += predicted;
        metric.prediction_square_sum += predicted * predicted;
        metric.target_sum += realized;
        metric.target_square_sum += realized * realized;
        metric.cross_sum += predicted * realized;
        if (sign(predicted) == sign(realized)) {
          ++metric.direction_correct;
        }
        if (p[anchor][edge][channel] >
            p[anchor][predicted_best][channel]) {
          predicted_best = edge;
        }
        if (t[anchor][edge][channel] > t[anchor][realized_best][channel]) {
          realized_best = edge;
        }
      }
      ++metric.best_count;
      if (predicted_best == realized_best) {
        ++metric.best_correct;
      }
      for (int64_t lhs = 0; lhs < kEdgeCount; ++lhs) {
        for (int64_t rhs = lhs + 1; rhs < kEdgeCount; ++rhs) {
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

MetricSummary summarize(const Metric &metric) {
  if (metric.count <= 0 || metric.pair_count <= 0 || metric.best_count <= 0) {
    throw std::runtime_error("empty metric domain");
  }
  const double count = static_cast<double>(metric.count);
  const double target_rms = std::sqrt(metric.target_square_sum / count);
  const double prediction_rms =
      std::sqrt(metric.prediction_square_sum / count);
  const double numerator =
      metric.cross_sum - metric.prediction_sum * metric.target_sum / count;
  const double prediction_variance =
      metric.prediction_square_sum -
      metric.prediction_sum * metric.prediction_sum / count;
  const double target_variance =
      metric.target_square_sum - metric.target_sum * metric.target_sum / count;
  const double correlation =
      prediction_variance > 0.0 && target_variance > 0.0
          ? numerator / std::sqrt(prediction_variance * target_variance)
          : 0.0;
  const double rmse = std::sqrt(metric.square_error_sum / count);
  return {.count = metric.count,
          .pair_count = metric.pair_count,
          .mae = metric.abs_error_sum / count,
          .rmse = rmse,
          .target_rms = target_rms,
          .prediction_rms = prediction_rms,
          .rmse_target_rms_ratio =
              target_rms > 0.0 ? rmse / target_rms
                               : std::numeric_limits<double>::infinity(),
          .direction = static_cast<double>(metric.direction_correct) / count,
          .rank = static_cast<double>(metric.pair_correct) /
                  static_cast<double>(metric.pair_count),
          .best_asset = static_cast<double>(metric.best_correct) /
                        static_cast<double>(metric.best_count),
          .correlation = correlation};
}

torch::Tensor predict(const Model &model, const torch::Tensor &features) {
  torch::NoGradGuard no_grad;
  const auto feature_width = model.feature_width;
  if (feature_width <= 0 || features.size(-1) != feature_width) {
    throw std::runtime_error("model and feature widths differ");
  }
  const auto standardized =
      (features - model.mean.view({1, 1, 1, feature_width})) *
      model.inv_std.view({1, 1, 1, feature_width});
  return (standardized *
          model.weights.view({1, kEdgeCount, 1, feature_width}))
             .sum(-1) +
         model.bias.view({1, kEdgeCount, 1});
}

Model fit(const Dataset &dataset, double ridge) {
  if (!(ridge > 0.0) || !std::isfinite(ridge)) {
    throw std::runtime_error("ridge must be positive and finite");
  }
  torch::NoGradGuard no_grad;
  if (dataset.anchor_begin != kTrainBegin ||
      dataset.anchor_end != kTrainEnd) {
    throw std::runtime_error("fit dataset is not exact train core");
  }
  const auto features = dataset.features;
  const auto target = dataset.target;
  const auto feature_width = features.size(-1);
  if (feature_width <= 0) {
    throw std::runtime_error("empty affine feature width");
  }
  const auto flat = features.reshape({-1, feature_width});
  const auto mean = flat.mean(0);
  const auto variance = (flat - mean).pow(2).mean(0);
  const auto standard_deviation = variance.sqrt();
  const auto inv_std = torch::where(standard_deviation > 1.0e-12,
                                    standard_deviation.reciprocal(),
                                    torch::ones_like(standard_deviation));
  const auto standardized =
      (features - mean.view({1, 1, 1, feature_width})) *
      inv_std.view({1, 1, 1, feature_width});
  auto weights = torch::zeros({kEdgeCount, feature_width}, torch::kFloat64);
  auto bias = torch::zeros({kEdgeCount}, torch::kFloat64);
  double maximum_residual = 0.0;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto x = standardized.select(1, edge).contiguous().reshape(
        {(kTrainEnd - kTrainBegin) * kChannelCount, feature_width});
    const auto y = target.select(1, edge).contiguous().reshape({-1});
    const auto x_mean = x.mean(0);
    const auto y_mean = y.mean();
    const auto centered_x = x - x_mean;
    const auto centered_y = y - y_mean;
    auto gram = centered_x.transpose(0, 1).matmul(centered_x);
    gram.diagonal(0, 0, 1).add_(static_cast<double>(x.size(0)) * ridge);
    const auto rhs = centered_x.transpose(0, 1).matmul(centered_y.unsqueeze(1));
    auto [cholesky, info] = at::linalg_cholesky_ex(gram, false, false);
    if (info.item<int64_t>() != 0) {
      throw CandidateNumericalError("cholesky_factorization_failed");
    }
    const auto row = at::cholesky_solve(rhs, cholesky, false).squeeze(1);
    const auto residual = gram.matmul(row.unsqueeze(1)) - rhs;
    const double normalized_residual =
        residual.norm().item<double>() /
        std::max(rhs.norm().item<double>(), 1.0e-30);
    if (!std::isfinite(normalized_residual) || normalized_residual > 1.0e-7 ||
        !torch::isfinite(row).all().item<bool>()) {
      throw CandidateNumericalError("normalized_residual_or_finiteness_failed");
    }
    maximum_residual = std::max(maximum_residual, normalized_residual);
    weights.select(0, edge).copy_(row);
    bias.select(0, edge).copy_(y_mean - x_mean.dot(row));
  }
  return {.mean = mean.contiguous(),
          .inv_std = inv_std.contiguous(),
          .weights = weights.contiguous(),
          .bias = bias.contiguous(),
          .feature_width = feature_width,
          .ridge = ridge,
          .maximum_normalized_residual = maximum_residual,
          .coefficient_l2_norm = weights.norm().item<double>()};
}

MetricSummary evaluate_dataset(const Dataset &dataset, const Model &model) {
  return summarize(observe(predict(model, dataset.features), dataset.target));
}

MetricSummary evaluate_channel(const Dataset &dataset, const Model &model,
                               int64_t channel) {
  return summarize(observe(
      predict(model, dataset.features).narrow(2, channel, 1),
      dataset.target.narrow(2, channel, 1)));
}

bool better(const Candidate &candidate, const Candidate &incumbent) {
  if (!candidate.numerically_valid || !incumbent.numerically_valid) {
    throw std::runtime_error("attempted to compare an invalid ridge candidate");
  }
  const auto compare_high = [](double lhs, double rhs) {
    if (lhs > rhs + kTieTolerance) {
      return 1;
    }
    if (rhs > lhs + kTieTolerance) {
      return -1;
    }
    return 0;
  };
  const std::array comparisons{
      std::pair{candidate.validation.direction,
                incumbent.validation.direction},
      std::pair{candidate.validation.rank, incumbent.validation.rank},
      std::pair{candidate.validation.correlation,
                incumbent.validation.correlation}};
  for (const auto &[lhs, rhs] : comparisons) {
    const int comparison = compare_high(lhs, rhs);
    if (comparison != 0) {
      return comparison > 0;
    }
  }
  if (candidate.validation.rmse + kTieTolerance <
      incumbent.validation.rmse) {
    return true;
  }
  if (incumbent.validation.rmse + kTieTolerance <
      candidate.validation.rmse) {
    return false;
  }
  return candidate.model.ridge < incumbent.model.ridge;
}

void emit_metric(std::ostream &output, const std::string &prefix,
                 const MetricSummary &metric) {
  output << prefix << ".count=" << metric.count << '\n';
  output << prefix << ".pairwise_rank_count=" << metric.pair_count << '\n';
  output << prefix << ".mae=" << metric.mae << '\n';
  output << prefix << ".rmse=" << metric.rmse << '\n';
  output << prefix << ".target_rms=" << metric.target_rms << '\n';
  output << prefix << ".prediction_rms=" << metric.prediction_rms << '\n';
  output << prefix << ".rmse_target_rms_ratio="
         << metric.rmse_target_rms_ratio << '\n';
  output << prefix << ".directional_accuracy=" << metric.direction << '\n';
  output << prefix << ".pairwise_rank_accuracy=" << metric.rank << '\n';
  output << prefix << ".best_asset_agreement=" << metric.best_asset << '\n';
  output << prefix << ".correlation=" << metric.correlation << '\n';
}

bool strong_gate(const MetricSummary &metric) {
  return metric.direction >= 0.95 && metric.rank >= 0.95 &&
         metric.correlation >= 0.95 && metric.rmse_target_rms_ratio <= 0.25;
}

bool partial_gate(const MetricSummary &metric) {
  return metric.direction >= 0.80 && metric.rank >= 0.78;
}

std::string classification(const MetricSummary &validation,
                           const MetricSummary &certified) {
  if (strong_gate(validation) && strong_gate(certified)) {
    return "strong_information_preservation";
  }
  if (partial_gate(validation) && partial_gate(certified)) {
    return "partial_information_preservation";
  }
  return "representation_or_exposed_interface_failure";
}

struct SelectionLockMetadata {
  bool verified{false};
  std::string schema_id;
  std::string probe_kind;
  int64_t selected_candidate_index{-1};
  double selected_ridge{0.0};
};

struct SelectionLockDocument {
  std::map<std::string, std::string> fields;
  std::set<std::string> consumed;

  void expect(const std::string &key, const std::string &expected) {
    const auto found = fields.find(key);
    if (found == fields.end()) {
      throw std::runtime_error("selection lock is missing key: " + key);
    }
    if (found->second != expected) {
      throw std::runtime_error("selection lock mismatch for key: " + key);
    }
    consumed.insert(key);
  }

  void reject_unconsumed_selection_fields() const {
    for (const auto &[key, value] : fields) {
      (void)value;
      const bool selection_field =
          key == "numerically_valid_candidate_count" ||
          key == "validation_strong_gate_pass" ||
          key == "validation_partial_gate_pass" ||
          key.rfind("candidate.", 0) == 0 ||
          key.rfind("selected_", 0) == 0 || key.rfind("selected.", 0) == 0;
      if (selection_field && consumed.find(key) == consumed.end()) {
        throw std::runtime_error(
            "selection lock contains an unexpected selection key: " + key);
      }
    }
  }
};

bool valid_selection_lock_key(const std::string &key) {
  if (key.empty()) {
    return false;
  }
  return std::all_of(key.begin(), key.end(), [](char cell) {
    const auto value = static_cast<unsigned char>(cell);
    return std::isalnum(value) != 0 || cell == '_' || cell == '.';
  });
}

SelectionLockDocument
read_selection_lock(const std::filesystem::path &path) {
  std::error_code status_error;
  const auto status = std::filesystem::symlink_status(path, status_error);
  if (status_error || std::filesystem::is_symlink(status) ||
      !std::filesystem::is_regular_file(status)) {
    throw std::runtime_error(
        "selection lock must be a regular non-symlinked file");
  }
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open selection lock");
  }
  input.imbue(std::locale::classic());
  SelectionLockDocument document;
  std::string line;
  std::size_t line_number = 0;
  while (std::getline(input, line)) {
    ++line_number;
    if (line.empty() || line.size() > (1U << 20U)) {
      throw std::runtime_error(
          "selection lock contains an empty or oversized line at line " +
          std::to_string(line_number));
    }
    const auto separator = line.find('=');
    if (separator == std::string::npos || separator == 0) {
      throw std::runtime_error("selection lock has a malformed line at line " +
                               std::to_string(line_number));
    }
    const auto key = line.substr(0, separator);
    if (!valid_selection_lock_key(key)) {
      throw std::runtime_error("selection lock has a malformed key at line " +
                               std::to_string(line_number));
    }
    const auto [position, inserted] =
        document.fields.emplace(key, line.substr(separator + 1));
    (void)position;
    if (!inserted) {
      throw std::runtime_error("selection lock contains duplicate key: " +
                               key);
    }
    if (document.fields.size() > 4096) {
      throw std::runtime_error("selection lock contains too many keys");
    }
  }
  if (!input.good() && !input.eof()) {
    throw std::runtime_error("failed while reading selection lock");
  }
  if (document.fields.empty()) {
    throw std::runtime_error("selection lock is empty");
  }
  return document;
}

std::string report_f64(double value) {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::setprecision(17) << value;
  if (!output) {
    throw std::runtime_error("failed to format selection lock value");
  }
  return output.str();
}

std::string report_bool(bool value) { return value ? "true" : "false"; }

void verify_locked_metric(SelectionLockDocument &lock,
                          const std::string &prefix,
                          const MetricSummary &metric) {
  lock.expect(prefix + ".count", std::to_string(metric.count));
  lock.expect(prefix + ".pairwise_rank_count",
              std::to_string(metric.pair_count));
  lock.expect(prefix + ".mae", report_f64(metric.mae));
  lock.expect(prefix + ".rmse", report_f64(metric.rmse));
  lock.expect(prefix + ".target_rms", report_f64(metric.target_rms));
  lock.expect(prefix + ".prediction_rms", report_f64(metric.prediction_rms));
  lock.expect(prefix + ".rmse_target_rms_ratio",
              report_f64(metric.rmse_target_rms_ratio));
  lock.expect(prefix + ".directional_accuracy", report_f64(metric.direction));
  lock.expect(prefix + ".pairwise_rank_accuracy", report_f64(metric.rank));
  lock.expect(prefix + ".best_asset_agreement",
              report_f64(metric.best_asset));
  lock.expect(prefix + ".correlation", report_f64(metric.correlation));
}

SelectionLockMetadata verify_selection_lock(
    const std::filesystem::path &path, const ProbeSpec &spec,
    const Dataset &train_dataset, const Dataset &validation_dataset,
    const std::vector<Candidate> &candidates, int64_t selected_index,
    const MetricSummary &train, const MetricSummary &validation,
    const std::array<MetricSummary, kChannelCount> &validation_channels) {
  auto lock = read_selection_lock(path);
  const std::string expected_schema =
      spec.kind == ProbeKind::kMdnContext
          ? "synthetic_v2_frozen_representation_affine_development_v1"
          : "synthetic_v2_frozen_encoder_affine_development_v1";
  lock.expect("schema_id", expected_schema);
  lock.expect("status", "complete");
  lock.expect("benchmark_id", "synthetic_continuous_graph_v2");
  lock.expect("probe_kind", std::string(spec.name));
  lock.expect("probe_record_schema", std::string(spec.record_schema));
  lock.expect("train_probe_rows", std::to_string(train_dataset.rows));
  lock.expect("validation_probe_rows",
              std::to_string(validation_dataset.rows));
  lock.expect("certified_probe_rows", "0");
  lock.expect("fit_anchor_range", "[0,2496)");
  lock.expect("validation_anchor_range", "[2560,2816)");
  lock.expect("certified_anchor_range", "not_opened");
  lock.expect("maximum_anchor_read", "2815");
  lock.expect("purge_anchors_used", "false");
  lock.expect("final_holdout_access", "false");
  lock.expect("policy_access", "false");
  lock.expect("refit_after_selection", "false");
  lock.expect("certified_candidates_scored", "0");
  lock.expect("affine_feature_width", std::to_string(spec.affine_width));
  lock.expect("edge_identity_feature_width_excluded",
              std::to_string(spec.probe_width - spec.affine_width));
  lock.expect("ridge_grid", "1e-12,1e-10,1e-8,1e-6,1e-4,1e-2");
  lock.expect("selection_order",
              "validation_direction,validation_rank,validation_correlation,"
              "validation_rmse,smallest_alpha");
  lock.expect("classification", "development_selection_complete");

  const auto valid_candidate_count =
      std::count_if(candidates.begin(), candidates.end(),
                    [](const Candidate &candidate) {
                      return candidate.numerically_valid;
                    });
  lock.expect("numerically_valid_candidate_count",
              std::to_string(valid_candidate_count));
  for (std::size_t index = 0; index < candidates.size(); ++index) {
    const auto &candidate = candidates[index];
    const auto prefix = "candidate." + std::to_string(index);
    lock.expect(prefix + ".ridge", report_f64(candidate.declared_ridge));
    lock.expect(prefix + ".numerically_valid",
                report_bool(candidate.numerically_valid));
    lock.expect(prefix + ".rejection_reason", candidate.rejection_reason);
    if (candidate.numerically_valid) {
      lock.expect(prefix + ".maximum_normalized_residual",
                  report_f64(candidate.model.maximum_normalized_residual));
      lock.expect(prefix + ".coefficient_l2_norm",
                  report_f64(candidate.model.coefficient_l2_norm));
      verify_locked_metric(lock, prefix + ".validation",
                           candidate.validation);
    }
  }

  if (selected_index < 0 ||
      selected_index >= static_cast<int64_t>(candidates.size())) {
    throw std::runtime_error("selected candidate index is outside the grid");
  }
  const auto &selected = candidates[static_cast<std::size_t>(selected_index)];
  if (!selected.numerically_valid) {
    throw std::runtime_error("selected candidate is numerically invalid");
  }
  lock.expect("selected_candidate_index", std::to_string(selected_index));
  lock.expect("selected_ridge", report_f64(selected.model.ridge));
  lock.expect("selected_maximum_normalized_residual",
              report_f64(selected.model.maximum_normalized_residual));
  lock.expect("selected_coefficient_l2_norm",
              report_f64(selected.model.coefficient_l2_norm));
  verify_locked_metric(lock, "selected.train", train);
  verify_locked_metric(lock, "selected.validation", validation);
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    verify_locked_metric(
        lock, "selected.validation.channel_" + std::to_string(channel),
        validation_channels[static_cast<std::size_t>(channel)]);
  }
  lock.expect("validation_strong_gate_pass",
              report_bool(strong_gate(validation)));
  lock.expect("validation_partial_gate_pass",
              report_bool(partial_gate(validation)));
  lock.reject_unconsumed_selection_fields();

  return {.verified = true,
          .schema_id = expected_schema,
          .probe_kind = std::string(spec.name),
          .selected_candidate_index = selected_index,
          .selected_ridge = selected.model.ridge};
}

void run(const Options &options) {
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  torch::manual_seed(31);
  if (options.probe_spec == nullptr) {
    throw std::runtime_error("probe specification is missing");
  }
  const auto &spec = *options.probe_spec;
  if (options.validation_only &&
      spec.kind != ProbeKind::kRepresentation) {
    throw std::runtime_error(
        "validation-only mode is reserved for the untrained representation "
        "control");
  }
  const bool has_certified =
      !options.validation_only && !options.development_only;
  if (has_certified && options.selection_lock.empty()) {
    throw std::runtime_error(
        "selection lock is required before certified evaluation");
  }
  const auto train_dataset =
      read_probe(options.train_input, kTrainBegin, kTrainEnd, spec);
  const auto validation_dataset = read_probe(
      options.validation_input, kValidationBegin, kValidationEnd, spec);

  std::vector<Candidate> candidates;
  candidates.reserve(kRidgeGrid.size());
  for (const double ridge : kRidgeGrid) {
    try {
      auto model = fit(train_dataset, ridge);
      auto validation = evaluate_dataset(validation_dataset, model);
      candidates.push_back({.numerically_valid = true,
                            .declared_ridge = ridge,
                            .rejection_reason = {},
                            .model = std::move(model),
                            .validation = validation});
    } catch (const CandidateNumericalError &error) {
      candidates.push_back({.numerically_valid = false,
                            .declared_ridge = ridge,
                            .rejection_reason = error.what(),
                            .model = {},
                            .validation = {}});
    }
  }
  auto selected = candidates.end();
  for (auto candidate = candidates.begin(); candidate != candidates.end();
       ++candidate) {
    if (!candidate->numerically_valid) {
      continue;
    }
    if (selected == candidates.end() || better(*candidate, *selected)) {
      selected = candidate;
    }
  }
  if (selected == candidates.end()) {
    throw std::runtime_error("no ridge candidate was selected");
  }

  const auto train = evaluate_dataset(train_dataset, selected->model);
  const auto validation = selected->validation;
  const auto selected_index =
      static_cast<int64_t>(std::distance(candidates.begin(), selected));
  std::array<MetricSummary, kChannelCount> locked_validation_channels{};
  SelectionLockMetadata selection_lock_metadata;
  if (has_certified) {
    for (int64_t channel = 0; channel < kChannelCount; ++channel) {
      locked_validation_channels[static_cast<std::size_t>(channel)] =
          evaluate_channel(validation_dataset, selected->model, channel);
    }
    selection_lock_metadata = verify_selection_lock(
        options.selection_lock, spec, train_dataset, validation_dataset,
        candidates, selected_index, train, validation,
        locked_validation_channels);
  }
  Dataset certified_dataset;
  MetricSummary certified;
  if (has_certified) {
    certified_dataset = read_probe(options.certified_input, kCertifiedBegin,
                                   kCertifiedEnd, spec);
    certified = evaluate_dataset(certified_dataset, selected->model);
  }

  std::ofstream output(options.output, std::ios::trunc);
  if (!output) {
    throw std::runtime_error("failed to open output report");
  }
  output.imbue(std::locale::classic());
  output << std::setprecision(17);
  std::string_view report_schema = spec.report_schema;
  if (options.validation_only) {
    report_schema = "synthetic_v2_untrained_encoder_affine_control_v1";
  } else if (options.development_only) {
    report_schema =
        spec.kind == ProbeKind::kMdnContext
            ? "synthetic_v2_frozen_representation_affine_development_v1"
            : "synthetic_v2_frozen_encoder_affine_development_v1";
  }
  output << "schema_id=" << report_schema << '\n';
  output << "status=complete\n";
  output << "benchmark_id=synthetic_continuous_graph_v2\n";
  output << "probe_kind=" << spec.name << '\n';
  output << "probe_record_schema=" << spec.record_schema << '\n';
  output << "train_probe_rows=" << train_dataset.rows << '\n';
  output << "validation_probe_rows=" << validation_dataset.rows << '\n';
  output << "certified_probe_rows="
         << (has_certified ? certified_dataset.rows : 0) << '\n';
  output << "probe_rows_total="
         << (train_dataset.rows + validation_dataset.rows +
             (has_certified ? certified_dataset.rows : 0))
         << '\n';
  output << "probe_ranges_disjoint=true\n";
  output << "fit_anchor_range=[0,2496)\n";
  output << "validation_anchor_range=[2560,2816)\n";
  output << "certified_anchor_range="
         << (has_certified ? "[2880,3261)" : "not_opened") << '\n';
  output << "purge_anchors_used=false\n";
  output << "maximum_anchor_read="
         << (has_certified ? kCertifiedEnd - 1 : kValidationEnd - 1)
         << '\n';
  output << "final_holdout_begin=3328\n";
  output << "final_holdout_access=false\n";
  output << "policy_access=false\n";
  output << "refit_after_selection=false\n";
  output << "certified_candidates_scored="
         << (has_certified ? 1 : 0) << '\n';
  if (has_certified) {
    const bool selection_lock_provided = !options.selection_lock.empty();
    output << "selection_lock_provided="
           << (selection_lock_provided ? "true" : "false") << '\n';
    output << "selection_lock_verified="
           << (selection_lock_metadata.verified ? "true" : "false") << '\n';
    if (selection_lock_provided) {
      output << "selection_lock_path=" << options.selection_lock.string()
             << '\n';
      output << "selection_lock_schema_id="
             << selection_lock_metadata.schema_id << '\n';
      output << "selection_lock_probe_kind="
             << selection_lock_metadata.probe_kind << '\n';
      output << "selection_lock_selected_candidate_index="
             << selection_lock_metadata.selected_candidate_index << '\n';
      output << "selection_lock_selected_ridge="
             << selection_lock_metadata.selected_ridge << '\n';
    }
  }
  output << "feature_layout=base_" << spec.representation_width
         << ",quote_" << spec.representation_width << ",base_minus_quote_"
         << spec.representation_width << '\n';
  output << "probe_feature_width=" << spec.probe_width << '\n';
  output << "affine_feature_width=" << spec.affine_width << '\n';
  output << "edge_identity_feature_width_excluded="
         << (spec.probe_width - spec.affine_width) << '\n';
  output << "fit_structure=one_weight_row_and_bias_per_edge_channels_pooled\n";
  output << "standardization_scope=train_core_all_edges_all_channels\n";
  output << "solver=float64_centered_cholesky_ridge\n";
  output << "ridge_scaling=gram_diagonal_plus_sample_count_times_alpha\n";
  output << "ridge_grid=1e-12,1e-10,1e-8,1e-6,1e-4,1e-2\n";
  output << "selection_order=validation_direction,validation_rank,"
            "validation_correlation,validation_rmse,smallest_alpha\n";
  output << "selection_tie_tolerance=" << kTieTolerance << '\n';
  output << "numerically_valid_candidate_count="
         << std::count_if(candidates.begin(), candidates.end(),
                          [](const Candidate &candidate) {
                            return candidate.numerically_valid;
                          })
         << '\n';
  const double maximum_identity_delta =
      has_certified
          ? std::max({train_dataset.context_identity_max_abs_delta,
                      validation_dataset.context_identity_max_abs_delta,
                      certified_dataset.context_identity_max_abs_delta})
          : std::max(train_dataset.context_identity_max_abs_delta,
                     validation_dataset.context_identity_max_abs_delta);
  output << "context_identity_max_abs_delta=" << maximum_identity_delta
         << '\n';

  for (std::size_t index = 0; index < candidates.size(); ++index) {
    const auto prefix = "candidate." + std::to_string(index);
    output << prefix << ".ridge=" << candidates[index].declared_ridge << '\n';
    output << prefix << ".numerically_valid="
           << (candidates[index].numerically_valid ? "true" : "false")
           << '\n';
    if (candidates[index].numerically_valid) {
      output << prefix << ".rejection_reason=\n";
      output << prefix << ".maximum_normalized_residual="
             << candidates[index].model.maximum_normalized_residual << '\n';
      output << prefix << ".coefficient_l2_norm="
             << candidates[index].model.coefficient_l2_norm << '\n';
      emit_metric(output, prefix + ".validation",
                  candidates[index].validation);
    } else {
      output << prefix << ".rejection_reason="
             << candidates[index].rejection_reason << '\n';
    }
  }

  output << "selected_candidate_index="
         << selected_index << '\n';
  output << "selected_ridge=" << selected->model.ridge << '\n';
  output << "selected_maximum_normalized_residual="
         << selected->model.maximum_normalized_residual << '\n';
  output << "selected_coefficient_l2_norm="
         << selected->model.coefficient_l2_norm << '\n';
  emit_metric(output, "selected.train", train);
  emit_metric(output, "selected.validation", validation);
  if (has_certified) {
    emit_metric(output, "selected.certified", certified);
  }
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    const auto validation_channel =
        selection_lock_metadata.verified
            ? locked_validation_channels[static_cast<std::size_t>(channel)]
            : evaluate_channel(validation_dataset, selected->model, channel);
    emit_metric(output,
                "selected.validation.channel_" + std::to_string(channel),
                validation_channel);
    if (has_certified) {
      emit_metric(output,
                  "selected.certified.channel_" + std::to_string(channel),
                  evaluate_channel(certified_dataset, selected->model,
                                   channel));
    }
  }
  output << "validation_strong_gate_pass="
         << (strong_gate(validation) ? "true" : "false") << '\n';
  output << "certified_strong_gate_pass="
         << (!has_certified
                 ? "not_evaluated"
                 : (strong_gate(certified) ? "true" : "false"))
         << '\n';
  output << "validation_partial_gate_pass="
         << (partial_gate(validation) ? "true" : "false") << '\n';
  output << "certified_partial_gate_pass="
         << (!has_certified
                 ? "not_evaluated"
                 : (partial_gate(certified) ? "true" : "false"))
         << '\n';
  output << "classification="
         << (options.validation_only
                 ? "untrained_representation_validation_control"
                 : (options.development_only
                        ? "development_selection_complete"
                        : classification(validation, certified)))
         << '\n';
  output << "preregistered_strong_gate=direction>=0.95,rank>=0.95,"
            "correlation>=0.95,rmse_target_rms_ratio<=0.25\n";
  output << "preregistered_partial_gate=direction>=0.80,rank>=0.78\n";
  output.flush();
  if (!output) {
    throw std::runtime_error("failed while writing output report");
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    std::locale::global(std::locale::classic());
    run(parse_options(argc, argv));
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "frozen representation affine probe: " << error.what()
              << '\n';
    return 1;
  }
}
