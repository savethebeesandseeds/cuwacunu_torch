// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

constexpr int64_t kTrainBegin = 0;
constexpr int64_t kTrainEnd = 2496;
constexpr int64_t kValidationBegin = 2560;
constexpr int64_t kValidationEnd = 2816;
constexpr int64_t kCertifiedEvalBegin = 2880;
constexpr int64_t kCertifiedEvalEnd = 3264;
constexpr int64_t kTestBegin = 3328;

// Audited production mapping for the daily master channel:
//   anchor 0 current close = daily row 29
//   anchor a target return = log(close[a + 30] / close[a + 29])
constexpr int64_t kCurrentCloseOffset = 29;
constexpr int64_t kTargetCloseOffset = 30;
constexpr int64_t kExpectedDailyPrefixRows = 3294;
constexpr int64_t kExpectedThreeDayPrefixRows = 1098;
constexpr int64_t kExpectedWeeklyPrefixRows = 471;
constexpr int64_t kMaxAnchorRead = kCertifiedEvalEnd - 1;
constexpr int64_t kMaxDailyRowRead = kMaxAnchorRead + kTargetCloseOffset;

constexpr int64_t kAssetCount = 3;
constexpr int64_t kMaximumAvailableLag = kCurrentCloseOffset;
constexpr int64_t kDefaultArOrder = 24;
constexpr double kDefaultRidgeLambda = 1.0e-8;
constexpr double kRidgeDirectionGate = 0.95;
constexpr double kRidgePairwiseRankGate = 0.95;
constexpr double kRidgeCorrelationGate = 0.95;
constexpr double kRidgeRmseToTargetRmsGate = 0.25;

constexpr std::array<std::string_view, kAssetCount> kAssetIds{
    "SYN2ALPHA", "SYN2BETA", "SYN2GAMMA"};

[[noreturn]] void fail(const std::string &message) {
  throw std::runtime_error(message);
}

void require(bool condition, const std::string &message) {
  if (!condition) {
    fail(message);
  }
}

struct Options {
  std::filesystem::path alpha_prefix;
  std::filesystem::path beta_prefix;
  std::filesystem::path gamma_prefix;
  std::filesystem::path output;
  int64_t expected_prefix_rows{kExpectedDailyPrefixRows};
  int64_t ar_order{kDefaultArOrder};
  double ridge_lambda{kDefaultRidgeLambda};
  std::optional<std::array<int64_t, kAssetCount>> authored_periods;
};

std::string required_value(int argc, char **argv, int &index,
                           const std::string &flag) {
  if (index + 1 >= argc) {
    fail("missing value for " + flag);
  }
  return argv[++index];
}

int64_t parse_int64(const std::string &text, const std::string &label) {
  int64_t value = 0;
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), value);
  require(result.ec == std::errc{} && result.ptr == text.data() + text.size(),
          "invalid integer for " + label + ": " + text);
  return value;
}

double parse_double(const std::string &text, const std::string &label) {
  errno = 0;
  char *end = nullptr;
  const double value = std::strtod(text.c_str(), &end);
  require(errno == 0 && end == text.c_str() + text.size() &&
              std::isfinite(value),
          "invalid finite number for " + label + ": " + text);
  return value;
}

std::array<int64_t, kAssetCount> parse_periods(const std::string &text) {
  std::array<int64_t, kAssetCount> periods{};
  std::size_t begin = 0;
  for (std::size_t index = 0; index < periods.size(); ++index) {
    const std::size_t end = text.find(',', begin);
    const bool final = index + 1 == periods.size();
    require(final ? end == std::string::npos : end != std::string::npos,
            "--authored-periods must contain exactly three comma-separated "
            "integers");
    const std::string token = text.substr(begin, end - begin);
    periods[index] = parse_int64(token, "authored period");
    require(periods[index] >= 1 && periods[index] <= kMaximumAvailableLag,
            "authored periods must be in [1,29]");
    begin = end == std::string::npos ? text.size() : end + 1;
  }
  return periods;
}

Options parse_options(int argc, char **argv) {
  Options options{};
  for (int index = 1; index < argc; ++index) {
    const std::string arg = argv[index];
    if (arg == "--alpha-prefix") {
      options.alpha_prefix = required_value(argc, argv, index, arg);
    } else if (arg == "--beta-prefix") {
      options.beta_prefix = required_value(argc, argv, index, arg);
    } else if (arg == "--gamma-prefix") {
      options.gamma_prefix = required_value(argc, argv, index, arg);
    } else if (arg == "--output") {
      options.output = required_value(argc, argv, index, arg);
    } else if (arg == "--expected-prefix-rows") {
      options.expected_prefix_rows =
          parse_int64(required_value(argc, argv, index, arg), arg);
    } else if (arg == "--ar-order") {
      options.ar_order =
          parse_int64(required_value(argc, argv, index, arg), arg);
    } else if (arg == "--ridge-lambda") {
      options.ridge_lambda =
          parse_double(required_value(argc, argv, index, arg), arg);
    } else if (arg == "--authored-periods") {
      options.authored_periods =
          parse_periods(required_value(argc, argv, index, arg));
    } else {
      fail("unknown argument: " + arg);
    }
  }

  require(!options.alpha_prefix.empty(), "--alpha-prefix is required");
  require(!options.beta_prefix.empty(), "--beta-prefix is required");
  require(!options.gamma_prefix.empty(), "--gamma-prefix is required");
  require(!options.output.empty(), "--output is required");
  require(options.expected_prefix_rows == kExpectedDailyPrefixRows,
          "the audited daily development prefix must contain exactly 3294 "
          "rows");
  require(options.ar_order >= 1 && options.ar_order <= kMaximumAvailableLag,
          "--ar-order must be in [1,29]");
  require(options.ridge_lambda >= 0.0 && std::isfinite(options.ridge_lambda),
          "--ridge-lambda must be finite and nonnegative");
  require(kMaxDailyRowRead == kExpectedDailyPrefixRows - 1,
          "internal daily prefix boundary mismatch");
  require(kMaxAnchorRead < kTestBegin,
          "internal anchor boundary reaches the test interval");
  require(kMaxDailyRowRead < kTestBegin,
          "internal daily-row boundary reaches the sealed final boundary");
  return options;
}

std::vector<std::string> split_csv_row(const std::string &line) {
  std::vector<std::string> fields;
  std::size_t begin = 0;
  while (true) {
    const std::size_t end = line.find(',', begin);
    fields.push_back(line.substr(begin, end - begin));
    if (end == std::string::npos) {
      break;
    }
    begin = end + 1;
  }
  return fields;
}

struct Series {
  std::vector<int64_t> open_time;
  std::vector<int64_t> close_time;
  std::vector<double> close;
};

void require_regular_nonsymlink(const std::filesystem::path &path,
                                const std::string &label) {
  const auto status = std::filesystem::symlink_status(path);
  require(status.type() == std::filesystem::file_type::regular,
          label + " must be an existing regular, non-symlinked file: " +
              path.string());
}

Series read_series(const std::filesystem::path &path, int64_t expected_rows,
                   const std::string &label) {
  require_regular_nonsymlink(path, label);
  std::ifstream input(path, std::ios::binary);
  require(input.good(), "could not open " + label + ": " + path.string());

  Series series{};
  series.open_time.reserve(static_cast<std::size_t>(expected_rows));
  series.close_time.reserve(static_cast<std::size_t>(expected_rows));
  series.close.reserve(static_cast<std::size_t>(expected_rows));

  std::string line;
  int64_t row = 0;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    require(row < expected_rows,
            label + " contains rows beyond the audited development prefix");
    const auto fields = split_csv_row(line);
    require(fields.size() == 12,
            label + " row " + std::to_string(row) +
                " does not contain exactly 12 kline fields");
    const int64_t open_time =
        parse_int64(fields[0], label + " open_time row " + std::to_string(row));
    const double close =
        parse_double(fields[4], label + " close row " + std::to_string(row));
    const int64_t close_time = parse_int64(
        fields[6], label + " close_time row " + std::to_string(row));
    require(close > 0.0,
            label + " has a nonpositive close at row " + std::to_string(row));
    require(close_time >= open_time,
            label + " has close_time before open_time at row " +
                std::to_string(row));
    if (row > 0) {
      require(open_time > series.open_time.back(),
              label + " open_time is not strictly increasing at row " +
                  std::to_string(row));
      require(close_time > series.close_time.back(),
              label + " close_time is not strictly increasing at row " +
                  std::to_string(row));
    }
    series.open_time.push_back(open_time);
    series.close_time.push_back(close_time);
    series.close.push_back(close);
    ++row;
  }
  require(input.eof(), "I/O failure while reading " + label);
  require(row == expected_rows, label + " row count mismatch: expected " +
                                    std::to_string(expected_rows) +
                                    ", observed " + std::to_string(row));
  return series;
}

using Dataset = std::array<Series, kAssetCount>;

void validate_alignment(const Dataset &dataset) {
  for (int64_t row = 0; row < kExpectedDailyPrefixRows; ++row) {
    for (int64_t asset = 1; asset < kAssetCount; ++asset) {
      require(dataset[static_cast<std::size_t>(asset)]
                      .open_time[static_cast<std::size_t>(row)] ==
                  dataset[0].open_time[static_cast<std::size_t>(row)],
              "cross-asset open_time mismatch at daily row " +
                  std::to_string(row));
      require(dataset[static_cast<std::size_t>(asset)]
                      .close_time[static_cast<std::size_t>(row)] ==
                  dataset[0].close_time[static_cast<std::size_t>(row)],
              "cross-asset close_time mismatch at daily row " +
                  std::to_string(row));
    }
  }
}

double edge_return(const Dataset &dataset, int64_t asset, int64_t start_row) {
  require(asset >= 0 && asset < kAssetCount, "invalid asset index");
  require(start_row >= 0 && start_row + 1 < kExpectedDailyPrefixRows,
          "edge-return row is outside the development prefix");
  const auto &close = dataset[static_cast<std::size_t>(asset)].close;
  return std::log(close[static_cast<std::size_t>(start_row + 1)] /
                  close[static_cast<std::size_t>(start_row)]);
}

double target_return(const Dataset &dataset, int64_t asset, int64_t anchor) {
  require(anchor >= 0 && anchor < kCertifiedEvalEnd,
          "target anchor is outside the visible development intervals");
  return edge_return(dataset, asset, anchor + kCurrentCloseOffset);
}

double lagged_return(const Dataset &dataset, int64_t asset, int64_t anchor,
                     int64_t lag) {
  require(lag >= 1 && lag <= kMaximumAvailableLag,
          "lag is outside the causal daily history");
  return edge_return(dataset, asset, anchor + kCurrentCloseOffset - lag);
}

int sign_of(double value) { return value > 0.0 ? 1 : (value < 0.0 ? -1 : 0); }

struct Metric {
  int64_t count{0};
  int64_t direction_correct{0};
  int64_t pair_count{0};
  int64_t pair_correct{0};
  int64_t best_count{0};
  int64_t best_correct{0};
  double absolute_error_sum{0.0};
  double squared_error_sum{0.0};
  double prediction_sum{0.0};
  double target_sum{0.0};
  double prediction_squared_sum{0.0};
  double target_squared_sum{0.0};
  double cross_sum{0.0};

  void observe(const std::array<double, kAssetCount> &prediction,
               const std::array<double, kAssetCount> &target) {
    for (int64_t asset = 0; asset < kAssetCount; ++asset) {
      const double p = prediction[static_cast<std::size_t>(asset)];
      const double y = target[static_cast<std::size_t>(asset)];
      require(std::isfinite(p) && std::isfinite(y), "non-finite metric input");
      const double error = p - y;
      ++count;
      absolute_error_sum += std::abs(error);
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
    for (int64_t lhs = 0; lhs < kAssetCount; ++lhs) {
      for (int64_t rhs = lhs + 1; rhs < kAssetCount; ++rhs) {
        ++pair_count;
        const double predicted_difference =
            prediction[static_cast<std::size_t>(lhs)] -
            prediction[static_cast<std::size_t>(rhs)];
        const double target_difference = target[static_cast<std::size_t>(lhs)] -
                                         target[static_cast<std::size_t>(rhs)];
        if (sign_of(predicted_difference) == sign_of(target_difference)) {
          ++pair_correct;
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
    return count > 0 ? absolute_error_sum / static_cast<double>(count) : 0.0;
  }

  double rmse() const {
    return count > 0 ? std::sqrt(squared_error_sum / static_cast<double>(count))
                     : 0.0;
  }

  double directional_accuracy() const {
    return count > 0 ? static_cast<double>(direction_correct) /
                           static_cast<double>(count)
                     : 0.0;
  }

  double pairwise_rank_accuracy() const {
    return pair_count > 0 ? static_cast<double>(pair_correct) /
                                static_cast<double>(pair_count)
                          : 0.0;
  }

  double best_asset_agreement() const {
    return best_count > 0 ? static_cast<double>(best_correct) /
                                static_cast<double>(best_count)
                          : 0.0;
  }

  double target_std() const {
    if (count <= 0) {
      return 0.0;
    }
    const double n = static_cast<double>(count);
    return std::sqrt(std::max(0.0, target_squared_sum / n -
                                       (target_sum / n) * (target_sum / n)));
  }

  double target_rms() const {
    return count > 0
               ? std::sqrt(target_squared_sum / static_cast<double>(count))
               : 0.0;
  }

  double prediction_std() const {
    if (count <= 0) {
      return 0.0;
    }
    const double n = static_cast<double>(count);
    return std::sqrt(
        std::max(0.0, prediction_squared_sum / n -
                          (prediction_sum / n) * (prediction_sum / n)));
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

struct RidgeModel {
  std::vector<double> feature_mean;
  std::vector<double> feature_scale;
  std::vector<double> coefficient;
};

std::vector<double> solve_linear_system(std::vector<std::vector<double>> matrix,
                                        std::vector<double> rhs) {
  const std::size_t dimension = matrix.size();
  require(dimension > 0 && rhs.size() == dimension,
          "invalid linear-system dimensions");
  for (const auto &row : matrix) {
    require(row.size() == dimension, "linear system is not square");
  }

  for (std::size_t column = 0; column < dimension; ++column) {
    std::size_t pivot = column;
    double pivot_abs = std::abs(matrix[column][column]);
    for (std::size_t row = column + 1; row < dimension; ++row) {
      const double candidate = std::abs(matrix[row][column]);
      if (candidate > pivot_abs) {
        pivot = row;
        pivot_abs = candidate;
      }
    }
    require(pivot_abs > 1.0e-18,
            "ridge normal equation is numerically singular");
    if (pivot != column) {
      std::swap(matrix[pivot], matrix[column]);
      std::swap(rhs[pivot], rhs[column]);
    }
    const double diagonal = matrix[column][column];
    for (std::size_t entry = column; entry < dimension; ++entry) {
      matrix[column][entry] /= diagonal;
    }
    rhs[column] /= diagonal;

    for (std::size_t row = 0; row < dimension; ++row) {
      if (row == column) {
        continue;
      }
      const double factor = matrix[row][column];
      if (factor == 0.0) {
        continue;
      }
      for (std::size_t entry = column; entry < dimension; ++entry) {
        matrix[row][entry] -= factor * matrix[column][entry];
      }
      rhs[row] -= factor * rhs[column];
    }
  }
  return rhs;
}

RidgeModel fit_ridge(const Dataset &dataset, int64_t asset, int64_t order,
                     double ridge_lambda) {
  RidgeModel model{};
  model.feature_mean.assign(static_cast<std::size_t>(order), 0.0);
  model.feature_scale.assign(static_cast<std::size_t>(order), 1.0);

  const double sample_count = static_cast<double>(kTrainEnd - kTrainBegin);
  for (int64_t anchor = kTrainBegin; anchor < kTrainEnd; ++anchor) {
    for (int64_t lag = 1; lag <= order; ++lag) {
      model.feature_mean[static_cast<std::size_t>(lag - 1)] +=
          lagged_return(dataset, asset, anchor, lag);
    }
  }
  for (double &mean : model.feature_mean) {
    mean /= sample_count;
  }

  std::vector<double> squared_deviation(static_cast<std::size_t>(order), 0.0);
  for (int64_t anchor = kTrainBegin; anchor < kTrainEnd; ++anchor) {
    for (int64_t lag = 1; lag <= order; ++lag) {
      const double centered =
          lagged_return(dataset, asset, anchor, lag) -
          model.feature_mean[static_cast<std::size_t>(lag - 1)];
      squared_deviation[static_cast<std::size_t>(lag - 1)] +=
          centered * centered;
    }
  }
  for (int64_t feature = 0; feature < order; ++feature) {
    const double scale = std::sqrt(
        squared_deviation[static_cast<std::size_t>(feature)] / sample_count);
    require(scale > 1.0e-14 && std::isfinite(scale),
            "ridge feature has zero or non-finite train standard deviation");
    model.feature_scale[static_cast<std::size_t>(feature)] = scale;
  }

  const std::size_t dimension = static_cast<std::size_t>(order + 1);
  std::vector<std::vector<double>> normal(dimension,
                                          std::vector<double>(dimension, 0.0));
  std::vector<double> rhs(dimension, 0.0);
  std::vector<double> feature(dimension, 1.0);
  for (int64_t anchor = kTrainBegin; anchor < kTrainEnd; ++anchor) {
    for (int64_t lag = 1; lag <= order; ++lag) {
      const std::size_t index = static_cast<std::size_t>(lag);
      feature[index] = (lagged_return(dataset, asset, anchor, lag) -
                        model.feature_mean[index - 1]) /
                       model.feature_scale[index - 1];
    }
    const double target = target_return(dataset, asset, anchor);
    for (std::size_t row = 0; row < dimension; ++row) {
      rhs[row] += feature[row] * target;
      for (std::size_t column = 0; column < dimension; ++column) {
        normal[row][column] += feature[row] * feature[column];
      }
    }
  }
  const double penalty = ridge_lambda * sample_count;
  for (std::size_t index = 1; index < dimension; ++index) {
    normal[index][index] += penalty;
  }
  model.coefficient = solve_linear_system(std::move(normal), std::move(rhs));
  return model;
}

double ridge_prediction(const RidgeModel &model, const Dataset &dataset,
                        int64_t asset, int64_t anchor) {
  require(model.coefficient.size() == model.feature_mean.size() + 1,
          "invalid ridge model shape");
  double prediction = model.coefficient[0];
  for (std::size_t feature = 0; feature < model.feature_mean.size();
       ++feature) {
    const int64_t lag = static_cast<int64_t>(feature) + 1;
    const double standardized = (lagged_return(dataset, asset, anchor, lag) -
                                 model.feature_mean[feature]) /
                                model.feature_scale[feature];
    prediction += model.coefficient[feature + 1] * standardized;
  }
  return prediction;
}

std::array<double, kAssetCount> fit_target_means(const Dataset &dataset) {
  std::array<double, kAssetCount> mean{};
  for (int64_t anchor = kTrainBegin; anchor < kTrainEnd; ++anchor) {
    for (int64_t asset = 0; asset < kAssetCount; ++asset) {
      mean[static_cast<std::size_t>(asset)] +=
          target_return(dataset, asset, anchor);
    }
  }
  const double count = static_cast<double>(kTrainEnd - kTrainBegin);
  for (double &value : mean) {
    value /= count;
  }
  return mean;
}

std::array<int64_t, kAssetCount>
select_seasonal_lags_on_train(const Dataset &dataset) {
  std::array<int64_t, kAssetCount> selected{};
  for (int64_t asset = 0; asset < kAssetCount; ++asset) {
    int64_t best_lag = 1;
    double best_mse = std::numeric_limits<double>::infinity();
    for (int64_t lag = 1; lag <= kMaximumAvailableLag; ++lag) {
      double squared_error = 0.0;
      for (int64_t anchor = kTrainBegin; anchor < kTrainEnd; ++anchor) {
        const double error = lagged_return(dataset, asset, anchor, lag) -
                             target_return(dataset, asset, anchor);
        squared_error += error * error;
      }
      const double mse =
          squared_error / static_cast<double>(kTrainEnd - kTrainBegin);
      // Candidate lags are visited in ascending order, so exact ties retain
      // the smaller causal lag deterministically.
      if (mse < best_mse) {
        best_mse = mse;
        best_lag = lag;
      }
    }
    selected[static_cast<std::size_t>(asset)] = best_lag;
  }
  return selected;
}

struct Models {
  std::array<double, kAssetCount> fitted_mean{};
  std::array<int64_t, kAssetCount> selected_seasonal_lag{};
  std::array<RidgeModel, kAssetCount> ridge;
};

Models fit_models(const Dataset &dataset, int64_t ar_order,
                  double ridge_lambda) {
  Models models{};
  models.fitted_mean = fit_target_means(dataset);
  models.selected_seasonal_lag = select_seasonal_lags_on_train(dataset);
  for (int64_t asset = 0; asset < kAssetCount; ++asset) {
    models.ridge[static_cast<std::size_t>(asset)] =
        fit_ridge(dataset, asset, ar_order, ridge_lambda);
  }
  return models;
}

enum class Arm {
  kZero,
  kFittedMean,
  kLagOne,
  kSelectedSeasonalLag,
  kRidgeAr,
  kAuthoredPeriodOracle,
};

std::array<double, kAssetCount> predict(Arm arm, const Dataset &dataset,
                                        const Models &models,
                                        const Options &options,
                                        int64_t anchor) {
  std::array<double, kAssetCount> prediction{};
  for (int64_t asset = 0; asset < kAssetCount; ++asset) {
    const std::size_t index = static_cast<std::size_t>(asset);
    switch (arm) {
    case Arm::kZero:
      prediction[index] = 0.0;
      break;
    case Arm::kFittedMean:
      prediction[index] = models.fitted_mean[index];
      break;
    case Arm::kLagOne:
      prediction[index] = lagged_return(dataset, asset, anchor, 1);
      break;
    case Arm::kSelectedSeasonalLag:
      prediction[index] = lagged_return(dataset, asset, anchor,
                                        models.selected_seasonal_lag[index]);
      break;
    case Arm::kRidgeAr:
      prediction[index] =
          ridge_prediction(models.ridge[index], dataset, asset, anchor);
      break;
    case Arm::kAuthoredPeriodOracle:
      require(options.authored_periods.has_value(),
              "authored-period oracle requested without periods");
      prediction[index] = lagged_return(
          dataset, asset, anchor, options.authored_periods.value()[index]);
      break;
    }
  }
  return prediction;
}

Metric evaluate(Arm arm, const Dataset &dataset, const Models &models,
                const Options &options, int64_t begin, int64_t end) {
  require(begin >= 0 && begin < end && end <= kCertifiedEvalEnd,
          "evaluation range is outside the visible development intervals");
  Metric metric{};
  for (int64_t anchor = begin; anchor < end; ++anchor) {
    std::array<double, kAssetCount> target{};
    for (int64_t asset = 0; asset < kAssetCount; ++asset) {
      target[static_cast<std::size_t>(asset)] =
          target_return(dataset, asset, anchor);
    }
    metric.observe(predict(arm, dataset, models, options, anchor), target);
  }
  const int64_t anchor_count = end - begin;
  require(metric.count == anchor_count * kAssetCount,
          "edge metric count mismatch");
  require(metric.pair_count == anchor_count * 3,
          "pairwise-rank metric count mismatch");
  return metric;
}

void emit_metric(std::ostream &output, const std::string &prefix,
                 const Metric &metric) {
  output << prefix << ".count=" << metric.count << '\n';
  output << prefix << ".mae=" << metric.mae() << '\n';
  output << prefix << ".rmse=" << metric.rmse() << '\n';
  output << prefix << ".directional_accuracy=" << metric.directional_accuracy()
         << '\n';
  output << prefix << ".correlation=" << metric.correlation() << '\n';
  output << prefix << ".pairwise_rank_count=" << metric.pair_count << '\n';
  output << prefix
         << ".pairwise_rank_accuracy=" << metric.pairwise_rank_accuracy()
         << '\n';
  output << prefix << ".best_asset_agreement=" << metric.best_asset_agreement()
         << '\n';
  output << prefix << ".prediction_population_std=" << metric.prediction_std()
         << '\n';
  output << prefix << ".target_population_std=" << metric.target_std() << '\n';
  output << prefix << ".target_rms=" << metric.target_rms() << '\n';
  output << prefix << ".prediction_to_target_std_ratio="
         << (metric.target_std() > 0.0
                 ? metric.prediction_std() / metric.target_std()
                 : 0.0)
         << '\n';
  output << prefix << ".normalized_rmse="
         << (metric.target_std() > 0.0 ? metric.rmse() / metric.target_std()
                                       : 0.0)
         << '\n';
  output << prefix << ".rmse_to_target_rms="
         << (metric.target_rms() > 0.0 ? metric.rmse() / metric.target_rms()
                                       : 0.0)
         << '\n';
}

bool ridge_metric_gate_passes(const Metric &metric) {
  return metric.directional_accuracy() >= kRidgeDirectionGate &&
         metric.pairwise_rank_accuracy() >= kRidgePairwiseRankGate &&
         metric.correlation() >= kRidgeCorrelationGate &&
         metric.target_rms() > 0.0 &&
         metric.rmse() / metric.target_rms() <= kRidgeRmseToTargetRmsGate;
}

void emit_ridge_gate(std::ostream &output, const std::string &split,
                     const Metric &metric) {
  const bool direction_pass =
      metric.directional_accuracy() >= kRidgeDirectionGate;
  const bool rank_pass =
      metric.pairwise_rank_accuracy() >= kRidgePairwiseRankGate;
  const bool correlation_pass = metric.correlation() >= kRidgeCorrelationGate;
  const bool rmse_pass =
      metric.target_rms() > 0.0 &&
      metric.rmse() / metric.target_rms() <= kRidgeRmseToTargetRmsGate;
  output << "gate.ridge." << split
         << ".directional_accuracy_pass=" << (direction_pass ? "true" : "false")
         << '\n';
  output << "gate.ridge." << split
         << ".pairwise_rank_accuracy_pass=" << (rank_pass ? "true" : "false")
         << '\n';
  output << "gate.ridge." << split
         << ".correlation_pass=" << (correlation_pass ? "true" : "false")
         << '\n';
  output << "gate.ridge." << split
         << ".rmse_to_target_rms_pass=" << (rmse_pass ? "true" : "false")
         << '\n';
  output << "gate.ridge." << split << ".overall_pass="
         << (ridge_metric_gate_passes(metric) ? "true" : "false") << '\n';
}

void emit_arm(std::ostream &output, const std::string &name, Arm arm,
              const Dataset &dataset, const Models &models,
              const Options &options) {
  emit_metric(output, "arm." + name + ".train",
              evaluate(arm, dataset, models, options, kTrainBegin, kTrainEnd));
  emit_metric(output, "arm." + name + ".validation",
              evaluate(arm, dataset, models, options, kValidationBegin,
                       kValidationEnd));
  emit_metric(output, "arm." + name + ".certified_eval",
              evaluate(arm, dataset, models, options, kCertifiedEvalBegin,
                       kCertifiedEvalEnd));
}

void write_report(const Options &options, const Dataset &dataset,
                  const Models &models) {
  const auto parent = options.output.parent_path();
  if (!parent.empty()) {
    require(std::filesystem::exists(parent) &&
                std::filesystem::is_directory(parent),
            "output parent must already exist: " + parent.string());
  }
  std::ofstream output(options.output, std::ios::binary | std::ios::trunc);
  require(output.good(), "could not open output: " + options.output.string());
  output << std::fixed << std::setprecision(12);
  output << "schema_id=synthetic_v2_data_predictability_baselines.v1\n";
  output << "status=complete\n";
  output << "benchmark_id=synthetic_continuous_graph_v2\n";
  output << "authority=development_only_raw_data_predictability\n";
  output << "model_execution_used=false\n";
  output << "representation_used=false\n";
  output << "policy_used=false\n";
  output << "test_input_used=false\n";
  output << "quote_node_id=SYN2USD\n";
  output << "base_node_ids=SYN2ALPHA,SYN2BETA,SYN2GAMMA\n";
  output << "target_definition=log(close[anchor+30]/close[anchor+29])\n";
  output << "history_last_close_row_definition=anchor+29\n";
  output << "train_anchor_range=[0,2496)\n";
  output << "validation_anchor_range=[2560,2816)\n";
  output << "certified_eval_anchor_range=[2880,3264)\n";
  output << "forbidden_test_anchor_range=[3328,4096)\n";
  output << "expected_daily_prefix_rows=" << kExpectedDailyPrefixRows << '\n';
  output << "expected_3d_prefix_rows=" << kExpectedThreeDayPrefixRows << '\n';
  output << "expected_1w_prefix_rows=" << kExpectedWeeklyPrefixRows << '\n';
  output << "observed_daily_prefix_rows=" << dataset[0].close.size() << '\n';
  output << "max_anchor_read=" << kMaxAnchorRead << '\n';
  output << "max_daily_row_read=" << kMaxDailyRowRead << '\n';
  output << "test_boundary_assertion_passed="
         << (kMaxAnchorRead < kTestBegin && kMaxDailyRowRead < kTestBegin
                 ? "true"
                 : "false")
         << '\n';
  output << "cross_asset_timestamp_alignment_passed=true\n";
  output << "daily_open_time_begin=" << dataset[0].open_time.front() << '\n';
  output << "daily_open_time_end=" << dataset[0].open_time.back() << '\n';
  output << "ridge_ar_order=" << options.ar_order << '\n';
  output << "ridge_lambda=" << options.ridge_lambda << '\n';
  output << "ridge_objective=mean_squared_error_plus_lambda_times_l2\n";
  output << "ridge_statistics_scope=train_only_per_asset_per_lag\n";
  output << "gate.ridge.directional_accuracy_threshold=" << kRidgeDirectionGate
         << '\n';
  output << "gate.ridge.pairwise_rank_accuracy_threshold="
         << kRidgePairwiseRankGate << '\n';
  output << "gate.ridge.correlation_threshold=" << kRidgeCorrelationGate
         << '\n';
  output << "gate.ridge.rmse_to_target_rms_maximum="
         << kRidgeRmseToTargetRmsGate << '\n';
  for (int64_t asset = 0; asset < kAssetCount; ++asset) {
    const std::size_t index = static_cast<std::size_t>(asset);
    output << "train_fitted_mean." << kAssetIds[index] << '='
           << models.fitted_mean[index] << '\n';
    output << "train_selected_seasonal_lag." << kAssetIds[index] << '='
           << models.selected_seasonal_lag[index] << '\n';
  }
  if (options.authored_periods.has_value()) {
    output << "authored_period_oracle_status=enabled\n";
    output << "authored_periods=" << options.authored_periods.value()[0] << ','
           << options.authored_periods.value()[1] << ','
           << options.authored_periods.value()[2] << '\n';
  } else {
    output << "authored_period_oracle_status=not_requested\n";
    output << "authored_periods=\n";
  }

  emit_arm(output, "zero", Arm::kZero, dataset, models, options);
  emit_arm(output, "fitted_mean", Arm::kFittedMean, dataset, models, options);
  emit_arm(output, "lag1", Arm::kLagOne, dataset, models, options);
  emit_arm(output, "train_selected_seasonal_lag", Arm::kSelectedSeasonalLag,
           dataset, models, options);
  emit_arm(output, "ridge_ar", Arm::kRidgeAr, dataset, models, options);
  if (options.authored_periods.has_value()) {
    emit_arm(output, "authored_period_oracle", Arm::kAuthoredPeriodOracle,
             dataset, models, options);
  }
  const Metric ridge_validation =
      evaluate(Arm::kRidgeAr, dataset, models, options, kValidationBegin,
               kValidationEnd);
  const Metric ridge_certified_eval =
      evaluate(Arm::kRidgeAr, dataset, models, options, kCertifiedEvalBegin,
               kCertifiedEvalEnd);
  emit_ridge_gate(output, "validation", ridge_validation);
  emit_ridge_gate(output, "certified_eval", ridge_certified_eval);
  output << "raw_predictability_gate_passed="
         << (ridge_metric_gate_passes(ridge_validation) &&
                     ridge_metric_gate_passes(ridge_certified_eval)
                 ? "true"
                 : "false")
         << '\n';
  output.flush();
  require(output.good(),
          "failed while writing output: " + options.output.string());
}

int run(int argc, char **argv) {
  const Options options = parse_options(argc, argv);
  Dataset dataset{
      read_series(options.alpha_prefix, options.expected_prefix_rows,
                  "SYN2ALPHASYN2USD daily prefix"),
      read_series(options.beta_prefix, options.expected_prefix_rows,
                  "SYN2BETASYN2USD daily prefix"),
      read_series(options.gamma_prefix, options.expected_prefix_rows,
                  "SYN2GAMMASYN2USD daily prefix")};
  validate_alignment(dataset);
  const Models models =
      fit_models(dataset, options.ar_order, options.ridge_lambda);
  write_report(options, dataset, models);
  return 0;
}

} // namespace

int main(int argc, char **argv) {
  try {
    return run(argc, argv);
  } catch (const std::exception &error) {
    std::cerr << "data_predictability_baselines: " << error.what() << '\n';
    return 1;
  }
}
