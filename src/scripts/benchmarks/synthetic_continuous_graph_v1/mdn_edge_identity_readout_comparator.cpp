#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

struct Row {
  long long anchor_key{};
  int anchor_index{};
  int edge_index{};
  std::string edge_id;
  std::string base_node_id;
  std::string quote_node_id;
  int channel_index{};
  double target{};
  std::vector<double> features;
};

struct Dataset {
  std::vector<Row> rows;
  int min_anchor{0};
  int max_anchor{0};
  int feature_count{0};
};

struct FeatureStats {
  std::vector<double> mean;
  std::vector<double> inv_std;
};

struct LinearModel {
  std::vector<double> weights;
  bool solved{false};
  std::string failure;
};

struct Metric {
  long long count{0};
  long long sign_correct{0};
  long long margin_count{0};
  long long margin_sign_correct{0};
  long long pair_count{0};
  long long pair_correct{0};
  long long margin_pair_count{0};
  long long margin_pair_correct{0};
  long long best_count{0};
  long long best_correct{0};
  double abs_error{0.0};
  double sq_error{0.0};
  double pred_sum{0.0};
  double target_sum{0.0};
  double pred_sq_sum{0.0};
  double target_sq_sum{0.0};
  double cross_sum{0.0};
};

struct FitMatrix {
  int dim{0};
  std::vector<double> xtx;
  std::vector<double> xty;
  long long rows{0};

  explicit FitMatrix(int dim_)
      : dim(dim_), xtx(dim_ * dim_, 0.0), xty(dim_, 0.0) {}

  double &a(int r, int c) { return xtx[static_cast<std::size_t>(r) * dim + c]; }
};

struct Options {
  std::filesystem::path input_path;
  std::filesystem::path output_path;
  std::string schema_id{"synthetic_mdn_edge_identity_readout_comparator.v1"};
  double fit_fraction{0.70};
  double margin_eps{0.001};
  double rank_margin_eps{0.001};
  double ridge_lambda{1.0e-6};
  int max_features{0};
  bool standardize{true};
};

std::vector<std::string> split_csv(const std::string &line) {
  std::vector<std::string> out;
  std::string cell;
  std::stringstream ss(line);
  while (std::getline(ss, cell, ',')) {
    out.push_back(cell);
  }
  return out;
}

std::vector<double> split_features(const std::string &text, int max_features) {
  std::vector<double> values;
  std::string cell;
  std::stringstream ss(text);
  while (std::getline(ss, cell, ';')) {
    if (cell.empty()) {
      values.push_back(0.0);
    } else {
      values.push_back(std::stod(cell));
    }
    if (max_features > 0 && static_cast<int>(values.size()) >= max_features) {
      break;
    }
  }
  return values;
}

double parse_double_arg(const char *value, const char *name) {
  char *end = nullptr;
  const double out = std::strtod(value, &end);
  if (end == value || *end != '\0' || !std::isfinite(out)) {
    throw std::runtime_error(std::string("invalid ") + name + ": " + value);
  }
  return out;
}

int parse_int_arg(const char *value, const char *name) {
  char *end = nullptr;
  const long out = std::strtol(value, &end, 10);
  if (end == value || *end != '\0' || out < 0) {
    throw std::runtime_error(std::string("invalid ") + name + ": " + value);
  }
  return static_cast<int>(out);
}

Options parse_options(int argc, char **argv) {
  Options opt;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    auto require_value = [&](const char *name) -> const char * {
      if (i + 1 >= argc) {
        throw std::runtime_error(std::string("missing value for ") + name);
      }
      return argv[++i];
    };
    if (arg == "--input") {
      opt.input_path = require_value("--input");
    } else if (arg == "--output") {
      opt.output_path = require_value("--output");
    } else if (arg == "--schema-id") {
      opt.schema_id = require_value("--schema-id");
    } else if (arg == "--fit-fraction") {
      opt.fit_fraction =
          parse_double_arg(require_value("--fit-fraction"), "--fit-fraction");
    } else if (arg == "--margin-eps") {
      opt.margin_eps =
          parse_double_arg(require_value("--margin-eps"), "--margin-eps");
    } else if (arg == "--rank-margin-eps") {
      opt.rank_margin_eps = parse_double_arg(require_value("--rank-margin-eps"),
                                             "--rank-margin-eps");
    } else if (arg == "--ridge-lambda") {
      opt.ridge_lambda =
          parse_double_arg(require_value("--ridge-lambda"), "--ridge-lambda");
    } else if (arg == "--max-features") {
      opt.max_features =
          parse_int_arg(require_value("--max-features"), "--max-features");
    } else if (arg == "--no-standardize") {
      opt.standardize = false;
    } else {
      throw std::runtime_error("unknown argument: " + arg);
    }
  }
  if (opt.input_path.empty()) {
    throw std::runtime_error("--input is required");
  }
  if (opt.output_path.empty()) {
    throw std::runtime_error("--output is required");
  }
  if (!(opt.fit_fraction > 0.0 && opt.fit_fraction < 1.0)) {
    throw std::runtime_error("--fit-fraction must be in (0,1)");
  }
  if (opt.ridge_lambda < 0.0) {
    throw std::runtime_error("--ridge-lambda must be nonnegative");
  }
  return opt;
}

Dataset read_dataset(const Options &opt) {
  std::ifstream in(opt.input_path);
  if (!in) {
    throw std::runtime_error("failed to open input probe: " +
                             opt.input_path.string());
  }
  Dataset dataset;
  std::string line;
  bool header = true;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    if (header) {
      header = false;
      continue;
    }
    auto cols = split_csv(line);
    if (cols.size() < 12) {
      continue;
    }
    Row row;
    row.anchor_key = std::stoll(cols[1]);
    row.anchor_index = std::stoi(cols[2]);
    row.edge_index = std::stoi(cols[4]);
    row.edge_id = cols[5];
    row.base_node_id = cols[6];
    row.quote_node_id = cols[7];
    row.channel_index = std::stoi(cols[8]);
    row.target = std::stod(cols[9]);
    row.features = split_features(cols[11], opt.max_features);
    if (row.features.empty()) {
      continue;
    }
    if (dataset.rows.empty()) {
      dataset.min_anchor = row.anchor_index;
      dataset.max_anchor = row.anchor_index;
      dataset.feature_count = static_cast<int>(row.features.size());
    } else {
      dataset.min_anchor = std::min(dataset.min_anchor, row.anchor_index);
      dataset.max_anchor = std::max(dataset.max_anchor, row.anchor_index);
      if (dataset.feature_count != static_cast<int>(row.features.size())) {
        throw std::runtime_error("inconsistent feature count in probe rows");
      }
    }
    dataset.rows.push_back(std::move(row));
  }
  if (dataset.rows.empty()) {
    throw std::runtime_error("input probe has no usable rows");
  }
  return dataset;
}

bool is_fit_row(const Row &row, int fit_end_anchor) {
  return row.anchor_index < fit_end_anchor;
}

FeatureStats compute_feature_stats(const Dataset &dataset, int fit_end_anchor,
                                   bool standardize) {
  FeatureStats stats;
  stats.mean.assign(dataset.feature_count, 0.0);
  stats.inv_std.assign(dataset.feature_count, 1.0);
  if (!standardize) {
    return stats;
  }
  long long n = 0;
  std::vector<double> sq(dataset.feature_count, 0.0);
  for (const auto &row : dataset.rows) {
    if (!is_fit_row(row, fit_end_anchor)) {
      continue;
    }
    ++n;
    for (int i = 0; i < dataset.feature_count; ++i) {
      stats.mean[i] += row.features[i];
      sq[i] += row.features[i] * row.features[i];
    }
  }
  if (n <= 1) {
    return stats;
  }
  for (int i = 0; i < dataset.feature_count; ++i) {
    stats.mean[i] /= static_cast<double>(n);
    const double var = std::max(0.0, sq[i] / static_cast<double>(n) -
                                         stats.mean[i] * stats.mean[i]);
    const double stddev = std::sqrt(var);
    stats.inv_std[i] = stddev > 1.0e-12 ? 1.0 / stddev : 1.0;
  }
  return stats;
}

std::vector<double> feature_vector(const Row &row, const FeatureStats &stats,
                                   bool include_edge_id) {
  std::vector<double> x;
  x.reserve(1 + row.features.size() + (include_edge_id ? 3 : 0));
  x.push_back(1.0);
  for (std::size_t i = 0; i < row.features.size(); ++i) {
    x.push_back((row.features[i] - stats.mean[i]) * stats.inv_std[i]);
  }
  if (include_edge_id) {
    for (int edge = 0; edge < 3; ++edge) {
      x.push_back(row.edge_index == edge ? 1.0 : 0.0);
    }
  }
  return x;
}

void accumulate(FitMatrix &fit, const std::vector<double> &x, double y) {
  if (fit.dim != static_cast<int>(x.size())) {
    throw std::runtime_error("feature vector dimension mismatch");
  }
  ++fit.rows;
  for (int r = 0; r < fit.dim; ++r) {
    fit.xty[r] += x[r] * y;
    for (int c = 0; c < fit.dim; ++c) {
      fit.a(r, c) += x[r] * x[c];
    }
  }
}

LinearModel solve(FitMatrix fit, double ridge_lambda) {
  LinearModel model;
  model.weights.assign(fit.dim, 0.0);
  if (fit.rows <= 0) {
    model.failure = "no_fit_rows";
    return model;
  }
  for (int i = 1; i < fit.dim; ++i) {
    fit.a(i, i) += ridge_lambda;
  }

  for (int i = 0; i < fit.dim; ++i) {
    int pivot = i;
    double pivot_abs = std::fabs(fit.a(i, i));
    for (int row = i + 1; row < fit.dim; ++row) {
      const double candidate = std::fabs(fit.a(row, i));
      if (candidate > pivot_abs) {
        pivot = row;
        pivot_abs = candidate;
      }
    }
    if (pivot_abs < 1.0e-14) {
      model.failure = "singular_matrix";
      return model;
    }
    if (pivot != i) {
      for (int col = i; col < fit.dim; ++col) {
        std::swap(fit.a(i, col), fit.a(pivot, col));
      }
      std::swap(fit.xty[i], fit.xty[pivot]);
    }
    const double denom = fit.a(i, i);
    for (int col = i; col < fit.dim; ++col) {
      fit.a(i, col) /= denom;
    }
    fit.xty[i] /= denom;
    for (int row = 0; row < fit.dim; ++row) {
      if (row == i) {
        continue;
      }
      const double factor = fit.a(row, i);
      if (factor == 0.0) {
        continue;
      }
      for (int col = i; col < fit.dim; ++col) {
        fit.a(row, col) -= factor * fit.a(i, col);
      }
      fit.xty[row] -= factor * fit.xty[i];
    }
  }
  model.weights = std::move(fit.xty);
  model.solved = true;
  return model;
}

double predict(const LinearModel &model, const std::vector<double> &x) {
  double out = 0.0;
  const auto n = std::min(model.weights.size(), x.size());
  for (std::size_t i = 0; i < n; ++i) {
    out += model.weights[i] * x[i];
  }
  return out;
}

int sign(double x) {
  if (x > 0.0) {
    return 1;
  }
  if (x < 0.0) {
    return -1;
  }
  return 0;
}

void observe_single(Metric &metric, double pred, double target,
                    double margin_eps) {
  ++metric.count;
  metric.abs_error += std::fabs(pred - target);
  metric.sq_error += (pred - target) * (pred - target);
  metric.pred_sum += pred;
  metric.target_sum += target;
  metric.pred_sq_sum += pred * pred;
  metric.target_sq_sum += target * target;
  metric.cross_sum += pred * target;
  if (sign(pred) == sign(target)) {
    ++metric.sign_correct;
  }
  if (std::fabs(target) > margin_eps) {
    ++metric.margin_count;
    if (sign(pred) == sign(target)) {
      ++metric.margin_sign_correct;
    }
  }
}

int best_edge(const std::map<int, double> &values) {
  int best = -1;
  double best_value = -std::numeric_limits<double>::infinity();
  for (const auto &[edge, value] : values) {
    if (best < 0 || value > best_value) {
      best = edge;
      best_value = value;
    }
  }
  return best;
}

void observe_groups(
    Metric &metric,
    const std::map<std::pair<int, int>, std::map<int, double>> &pred,
    const std::map<std::pair<int, int>, std::map<int, double>> &target,
    double rank_margin_eps) {
  for (const auto &[key, pvalues] : pred) {
    const auto tit = target.find(key);
    if (tit == target.end()) {
      continue;
    }
    const auto &tvalues = tit->second;
    if (pvalues.size() < 3 || tvalues.size() < 3) {
      continue;
    }
    ++metric.best_count;
    if (best_edge(pvalues) == best_edge(tvalues)) {
      ++metric.best_correct;
    }
    for (int lhs = 0; lhs < 2; ++lhs) {
      for (int rhs = lhs + 1; rhs < 3; ++rhs) {
        const double pd = pvalues.at(lhs) - pvalues.at(rhs);
        const double td = tvalues.at(lhs) - tvalues.at(rhs);
        ++metric.pair_count;
        if (sign(pd) == sign(td)) {
          ++metric.pair_correct;
        }
        if (std::fabs(td) > rank_margin_eps) {
          ++metric.margin_pair_count;
          if (sign(pd) == sign(td)) {
            ++metric.margin_pair_correct;
          }
        }
      }
    }
  }
}

double corr(const Metric &metric) {
  if (metric.count <= 1) {
    return 0.0;
  }
  const double n = static_cast<double>(metric.count);
  const double numerator =
      metric.cross_sum - metric.pred_sum * metric.target_sum / n;
  const double left =
      metric.pred_sq_sum - metric.pred_sum * metric.pred_sum / n;
  const double right =
      metric.target_sq_sum - metric.target_sum * metric.target_sum / n;
  if (left <= 0.0 || right <= 0.0) {
    return 0.0;
  }
  return numerator / std::sqrt(left * right);
}

double pred_std(const Metric &metric) {
  if (metric.count <= 1) {
    return 0.0;
  }
  const double n = static_cast<double>(metric.count);
  const double var =
      metric.pred_sq_sum / n - (metric.pred_sum / n) * (metric.pred_sum / n);
  return std::sqrt(std::max(0.0, var));
}

double target_std(const Metric &metric) {
  if (metric.count <= 1) {
    return 0.0;
  }
  const double n = static_cast<double>(metric.count);
  const double var = metric.target_sq_sum / n -
                     (metric.target_sum / n) * (metric.target_sum / n);
  return std::sqrt(std::max(0.0, var));
}

void emit_metric(std::ostream &out, const std::string &prefix,
                 const Metric &metric) {
  out << prefix << ".count=" << metric.count << "\n";
  if (metric.count > 0) {
    const double n = static_cast<double>(metric.count);
    const double pstd = pred_std(metric);
    const double tstd = target_std(metric);
    out << prefix << ".mae=" << metric.abs_error / n << "\n";
    out << prefix << ".rmse=" << std::sqrt(metric.sq_error / n) << "\n";
    out << prefix << ".directional_accuracy="
        << static_cast<double>(metric.sign_correct) / n << "\n";
    out << prefix << ".correlation=" << corr(metric) << "\n";
    out << prefix << ".pred_std=" << pstd << "\n";
    out << prefix << ".target_std=" << tstd << "\n";
    out << prefix
        << ".pred_to_target_std_ratio=" << (tstd > 0.0 ? pstd / tstd : 0.0)
        << "\n";
  }
  out << prefix << ".margin_count=" << metric.margin_count << "\n";
  if (metric.margin_count > 0) {
    out << prefix << ".margin_directional_accuracy="
        << static_cast<double>(metric.margin_sign_correct) /
               static_cast<double>(metric.margin_count)
        << "\n";
  }
  out << prefix << ".pairwise_rank_count=" << metric.pair_count << "\n";
  if (metric.pair_count > 0) {
    out << prefix << ".pairwise_rank_accuracy="
        << static_cast<double>(metric.pair_correct) /
               static_cast<double>(metric.pair_count)
        << "\n";
  }
  out << prefix << ".margin_pairwise_rank_count=" << metric.margin_pair_count
      << "\n";
  if (metric.margin_pair_count > 0) {
    out << prefix << ".margin_pairwise_rank_accuracy="
        << static_cast<double>(metric.margin_pair_correct) /
               static_cast<double>(metric.margin_pair_count)
        << "\n";
  }
  out << prefix << ".best_asset_count=" << metric.best_count << "\n";
  if (metric.best_count > 0) {
    out << prefix << ".best_asset_agreement="
        << static_cast<double>(metric.best_correct) /
               static_cast<double>(metric.best_count)
        << "\n";
  }
}

std::vector<LinearModel> fit_variant(const Dataset &dataset, int fit_end_anchor,
                                     const FeatureStats &stats,
                                     bool include_edge_id, bool per_edge,
                                     double ridge_lambda) {
  const int dim = 1 + dataset.feature_count + (include_edge_id ? 3 : 0);
  if (!per_edge) {
    FitMatrix fit(dim);
    for (const auto &row : dataset.rows) {
      if (!is_fit_row(row, fit_end_anchor)) {
        continue;
      }
      accumulate(fit, feature_vector(row, stats, include_edge_id), row.target);
    }
    return {solve(std::move(fit), ridge_lambda)};
  }

  std::vector<FitMatrix> fits;
  for (int edge = 0; edge < 3; ++edge) {
    fits.emplace_back(dim);
  }
  for (const auto &row : dataset.rows) {
    if (!is_fit_row(row, fit_end_anchor) || row.edge_index < 0 ||
        row.edge_index >= 3) {
      continue;
    }
    accumulate(fits[static_cast<std::size_t>(row.edge_index)],
               feature_vector(row, stats, include_edge_id), row.target);
  }
  std::vector<LinearModel> models;
  for (auto &fit : fits) {
    models.push_back(solve(std::move(fit), ridge_lambda));
  }
  return models;
}

Metric evaluate_variant(const Dataset &dataset, int fit_end_anchor,
                        const FeatureStats &stats,
                        const std::vector<LinearModel> &models,
                        bool include_edge_id, bool per_edge, bool fit_split,
                        double margin_eps, double rank_margin_eps) {
  Metric metric;
  std::map<std::pair<int, int>, std::map<int, double>> group_pred;
  std::map<std::pair<int, int>, std::map<int, double>> group_target;
  for (const auto &row : dataset.rows) {
    if (is_fit_row(row, fit_end_anchor) != fit_split) {
      continue;
    }
    const int model_index = per_edge ? row.edge_index : 0;
    if (model_index < 0 || model_index >= static_cast<int>(models.size()) ||
        !models[static_cast<std::size_t>(model_index)].solved) {
      continue;
    }
    const auto x = feature_vector(row, stats, include_edge_id);
    const double pred =
        predict(models[static_cast<std::size_t>(model_index)], x);
    observe_single(metric, pred, row.target, margin_eps);
    const auto key = std::make_pair(row.anchor_index, row.channel_index);
    group_pred[key][row.edge_index] = pred;
    group_target[key][row.edge_index] = row.target;
  }
  observe_groups(metric, group_pred, group_target, rank_margin_eps);
  return metric;
}

std::string bool_string(bool value) { return value ? "true" : "false"; }

void emit_variant(std::ostream &out, const std::string &name,
                  const Dataset &dataset, int fit_end_anchor,
                  const FeatureStats &stats, bool include_edge_id,
                  bool per_edge, double ridge_lambda, double margin_eps,
                  double rank_margin_eps) {
  const auto models = fit_variant(dataset, fit_end_anchor, stats,
                                  include_edge_id, per_edge, ridge_lambda);
  bool all_solved = !models.empty();
  for (const auto &model : models) {
    all_solved = all_solved && model.solved;
  }
  out << "variant." << name
      << ".include_edge_id=" << bool_string(include_edge_id) << "\n";
  out << "variant." << name << ".per_edge=" << bool_string(per_edge) << "\n";
  out << "variant." << name << ".all_models_solved=" << bool_string(all_solved)
      << "\n";
  for (std::size_t i = 0; i < models.size(); ++i) {
    if (!models[i].solved) {
      out << "variant." << name << ".model_" << i
          << "_failure=" << models[i].failure << "\n";
    }
  }
  const auto fit_metric =
      evaluate_variant(dataset, fit_end_anchor, stats, models, include_edge_id,
                       per_edge, true, margin_eps, rank_margin_eps);
  const auto holdout_metric =
      evaluate_variant(dataset, fit_end_anchor, stats, models, include_edge_id,
                       per_edge, false, margin_eps, rank_margin_eps);
  emit_metric(out, "variant." + name + ".fit", fit_metric);
  emit_metric(out, "variant." + name + ".holdout", holdout_metric);
}

} // namespace

int main(int argc, char **argv) try {
  const auto opt = parse_options(argc, argv);
  const auto dataset = read_dataset(opt);
  const int anchor_count = dataset.max_anchor - dataset.min_anchor + 1;
  const int fit_anchor_count = std::max(
      1, static_cast<int>(std::floor(anchor_count * opt.fit_fraction)));
  const int fit_end_anchor = dataset.min_anchor + fit_anchor_count;
  if (fit_end_anchor > dataset.max_anchor) {
    throw std::runtime_error("fit split consumed all anchors");
  }
  const auto stats =
      compute_feature_stats(dataset, fit_end_anchor, opt.standardize);

  std::filesystem::create_directories(opt.output_path.parent_path());
  std::ofstream out(opt.output_path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error("failed to open output probe: " +
                             opt.output_path.string());
  }
  out << std::setprecision(12);
  out << "schema_id=" << opt.schema_id << "\n";
  out << "status=complete\n";
  out << "benchmark_id=synthetic_continuous_graph_v1\n";
  out << "input_probe_path=" << opt.input_path.string() << "\n";
  out << "input_probe_rows=" << dataset.rows.size() << "\n";
  out << "feature_count=" << dataset.feature_count << "\n";
  out << "max_features=" << opt.max_features << "\n";
  out << "standardize_features=" << bool_string(opt.standardize) << "\n";
  out << "anchor_range=[" << dataset.min_anchor << ","
      << (dataset.max_anchor + 1) << ")\n";
  out << "fit_anchor_range=[" << dataset.min_anchor << "," << fit_end_anchor
      << ")\n";
  out << "holdout_anchor_range=[" << fit_end_anchor << ","
      << (dataset.max_anchor + 1) << ")\n";
  out << "fit_fraction=" << opt.fit_fraction << "\n";
  out << "margin_eps=" << opt.margin_eps << "\n";
  out << "rank_margin_eps=" << opt.rank_margin_eps << "\n";
  out << "ridge_lambda=" << opt.ridge_lambda << "\n";
  out << "comparator_authority=diagnostic_only\n";

  emit_variant(out, "shared_no_edge_id_linear", dataset, fit_end_anchor, stats,
               false, false, opt.ridge_lambda, opt.margin_eps,
               opt.rank_margin_eps);
  emit_variant(out, "shared_edge_id_linear", dataset, fit_end_anchor, stats,
               true, false, opt.ridge_lambda, opt.margin_eps,
               opt.rank_margin_eps);
  emit_variant(out, "per_edge_linear", dataset, fit_end_anchor, stats, false,
               true, opt.ridge_lambda, opt.margin_eps, opt.rank_margin_eps);

  out << "interpretation.shared_no_id_fails_edge_id_succeeds=runtime_readout_"
         "needs_identity_or_specialization\n";
  out << "interpretation.all_variants_fail=signal_lost_before_direct_head_"
         "input\n";
  out << "interpretation.shared_no_id_succeeds=runtime_training_dynamics_or_"
         "objective_scheduling_suspect\n";
  return 0;
} catch (const std::exception &ex) {
  std::cerr << "[synthetic_mdn_edge_identity_readout_comparator] " << ex.what()
            << "\n";
  return 1;
}
