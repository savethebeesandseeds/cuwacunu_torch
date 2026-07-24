#include <algorithm>
#include <bit>
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
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <ATen/ops/cholesky_solve.h>
#include <ATen/ops/linalg_cholesky_ex.h>
#include <torch/torch.h>

#include "piaabo/digest/sha256.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network_head.h"
#include "wikimyei/inference/expected_value/mdn/per_edge_affine_return_head.h"

namespace {

namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;

struct Options {
  std::filesystem::path input_path;
  std::filesystem::path evaluation_input_path;
  std::filesystem::path output_path;
  std::filesystem::path affine_calibration_output_path;
  std::filesystem::path affine_calibration_metadata_output_path;
  std::string affine_calibration_source_schema;
  std::string affine_calibration_fit_probe_sha256;
  std::string affine_calibration_representation_checkpoint_sha256;
  std::string affine_calibration_mdn_checkpoint_sha256;
  std::string affine_calibration_graph_fingerprint;
  std::string affine_calibration_capture_authority;
  std::string schema_id{"synthetic_mdn_cached_feature_runtime_head_parity.v1"};
  std::vector<std::string> identity_modes{
      "shared", "edge_embedding", "per_edge", "edge_embedding_per_edge"};
  int64_t feature_dim{128};
  int64_t identity_embedding_dim{16};
  int64_t steps{3500};
  int64_t batch_size{64};
  int64_t overfit_anchors{16};
  int64_t overfit_steps{2000};
  int64_t standardized_linear_steps{3500};
  int64_t seed{31};
  double fit_fraction{0.70};
  std::vector<double> ridge_alphas{1.0e-10, 1.0e-9, 1.0e-8, 1.0e-7,
                                   1.0e-6,  1.0e-5, 1.0e-4, 1.0e-3,
                                   1.0e-2,  1.0e-1, 1.0,    10.0};
  double ridge_validation_fraction{0.20};
  int64_t ridge_validation_gap{0};
  double ridge_direction_gate{0.95};
  double ridge_rank_gate{0.95};
  double learning_rate{1.0e-3};
  double grad_clip_norm{5.0};
  double regression_weight{100.0};
  double direction_weight{5.0};
  double rank_weight{5.0};
  double huber_beta{0.5};
  double logit_scale{1.0};
  double target_scale{36.0};
  double margin_eps{0.001};
  double rank_margin_eps{0.001};
  bool force_cpu{false};
  bool ridge_only{false};
  bool affine_parity_only{false};
};

struct Row {
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
  torch::Tensor context;          // [A,N,C,H], reconstructed adapted context.
  torch::Tensor readout_features; // [A,E,C,F], exact cached readout input.
  torch::Tensor target;           // [A,E,C].
  std::vector<int> anchors;
  std::vector<std::string> edge_ids;
  std::vector<std::string> base_node_ids;
  std::string quote_node_id;
  int64_t edge_count{};
  int64_t channel_count{};
  int64_t feature_dim{};
  int64_t source_feature_count{};
  int64_t primary_anchor_count{};
  double reconstruction_max_abs_error{};
  double quote_consistency_max_abs_error{};
};

struct Objective {
  torch::Tensor total;
  torch::Tensor regression;
  torch::Tensor direction;
  torch::Tensor rank;
};

struct TrainSummary {
  double initial_loss{};
  double final_loss{};
};

struct Metric {
  int64_t count{};
  int64_t sign_correct{};
  int64_t margin_count{};
  int64_t margin_sign_correct{};
  int64_t pair_count{};
  int64_t pair_correct{};
  int64_t margin_pair_count{};
  int64_t margin_pair_correct{};
  int64_t best_count{};
  int64_t best_correct{};
  double abs_error{};
  double sq_error{};
  double pred_sum{};
  double target_sum{};
  double pred_sq_sum{};
  double target_sq_sum{};
  double cross_sum{};
};

struct StandardizedPerEdgeLinearHeadImpl : torch::nn::Module {
  int64_t H{0};
  int64_t edge_count{0};
  torch::Tensor feature_mean;
  torch::Tensor feature_inv_std;
  std::vector<torch::nn::Linear> edge_projections;

  StandardizedPerEdgeLinearHeadImpl(int64_t H_, int64_t edge_count_,
                                    const torch::Tensor &feature_mean_,
                                    const torch::Tensor &feature_inv_std_)
      : H(H_), edge_count(edge_count_) {
    TORCH_CHECK(H > 0 && edge_count > 0,
                "[StandardizedPerEdgeLinearHead] invalid dimensions");
    TORCH_CHECK(feature_mean_.defined() && feature_inv_std_.defined() &&
                    feature_mean_.numel() == 3 * H &&
                    feature_inv_std_.numel() == 3 * H,
                "[StandardizedPerEdgeLinearHead] invalid feature statistics");
    feature_mean = register_buffer(
        "feature_mean", feature_mean_.detach().clone().view({1, 1, 1, 3 * H}));
    feature_inv_std = register_buffer(
        "feature_inv_std",
        feature_inv_std_.detach().clone().view({1, 1, 1, 3 * H}));
    edge_projections.reserve(static_cast<std::size_t>(edge_count));
    for (int64_t edge = 0; edge < edge_count; ++edge) {
      auto projection = torch::nn::Linear(3 * H, 1);
      edge_projections.push_back(register_module(
          "projection_edge_" + std::to_string(edge), projection));
    }
    torch::NoGradGuard no_grad;
    for (auto &projection : edge_projections) {
      projection->weight.zero_();
      if (projection->bias.defined()) {
        projection->bias.zero_();
      }
    }
  }

  torch::Tensor edge_features(const torch::Tensor &h) const {
    TORCH_CHECK(h.defined() && h.dim() == 4 && h.size(1) == edge_count + 1 &&
                    h.size(3) == H,
                "[StandardizedPerEdgeLinearHead] context shape mismatch");
    const auto B = h.size(0);
    const auto C = h.size(2);
    auto quote = h.select(1, 0).unsqueeze(1).expand({B, edge_count, C, H});
    auto base = h.narrow(1, 1, edge_count);
    return torch::cat({base, quote, base - quote}, /*dim=*/-1).contiguous();
  }

  torch::Tensor forward(const torch::Tensor &h) {
    const auto output_options = h.options();
    const auto compute_h = h.to(feature_mean.options());
    const auto features = edge_features(compute_h);
    const auto standardized = (features - feature_mean) * feature_inv_std;
    const auto B = h.size(0);
    const auto C = h.size(2);
    using torch::indexing::Slice;
    std::vector<torch::Tensor> outputs;
    outputs.reserve(static_cast<std::size_t>(edge_count));
    for (int64_t edge = 0; edge < edge_count; ++edge) {
      auto input = standardized.index({Slice(), edge, Slice(), Slice()})
                       .contiguous()
                       .view({B * C, 3 * H});
      outputs.push_back(
          edge_projections[static_cast<std::size_t>(edge)]->forward(input).view(
              {B, 1, C}));
    }
    return torch::cat(outputs, /*dim=*/1).to(output_options);
  }
};
TORCH_MODULE(StandardizedPerEdgeLinearHead);

struct AnalyticRidgeHeadImpl : torch::nn::Module {
  int64_t H{0};
  int64_t edge_count{0};
  torch::Tensor feature_mean;
  torch::Tensor feature_inv_std;
  torch::Tensor weights;
  torch::Tensor bias;

  AnalyticRidgeHeadImpl(int64_t H_, int64_t edge_count_,
                        const torch::Tensor &feature_mean_,
                        const torch::Tensor &feature_inv_std_,
                        const torch::Tensor &weights_,
                        const torch::Tensor &bias_)
      : H(H_), edge_count(edge_count_) {
    const auto feature_count = 3 * H;
    TORCH_CHECK(H > 0 && edge_count > 0,
                "[AnalyticRidgeHead] invalid dimensions");
    TORCH_CHECK(feature_mean_.numel() == feature_count &&
                    feature_inv_std_.numel() == feature_count,
                "[AnalyticRidgeHead] invalid feature statistics");
    TORCH_CHECK(weights_.sizes() ==
                        torch::IntArrayRef({edge_count, feature_count}) &&
                    bias_.numel() == edge_count,
                "[AnalyticRidgeHead] invalid coefficients");
    feature_mean =
        register_buffer("feature_mean", feature_mean_.detach()
                                            .clone()
                                            .to(torch::kFloat64)
                                            .view({1, 1, 1, feature_count}));
    feature_inv_std =
        register_buffer("feature_inv_std", feature_inv_std_.detach()
                                               .clone()
                                               .to(torch::kFloat64)
                                               .view({1, 1, 1, feature_count}));
    weights = register_buffer("weights",
                              weights_.detach()
                                  .clone()
                                  .to(torch::kFloat64)
                                  .view({1, edge_count, 1, feature_count}));
    bias = register_buffer(
        "bias",
        bias_.detach().clone().to(torch::kFloat64).view({1, edge_count, 1}));
  }

  torch::Tensor forward(const torch::Tensor &h) {
    TORCH_CHECK(h.defined() && h.device().is_cpu() && h.dim() == 4 &&
                    h.size(1) == edge_count + 1 && h.size(3) == H,
                "[AnalyticRidgeHead] context shape or device mismatch");
    const auto B = h.size(0);
    const auto C = h.size(2);
    const auto h64 = h.to(torch::kFloat64);
    auto quote = h64.select(1, 0).unsqueeze(1).expand({B, edge_count, C, H});
    auto base = h64.narrow(1, 1, edge_count);
    auto features =
        torch::cat({base, quote, base - quote}, /*dim=*/-1).contiguous();
    auto standardized = (features - feature_mean) * feature_inv_std;
    return ((standardized * weights).sum(/*dim=*/-1) + bias)
        .to(h.scalar_type());
  }
};
TORCH_MODULE(AnalyticRidgeHead);

struct AnalyticRidgeFit {
  AnalyticRidgeHead head{nullptr};
  double max_normalized_residual{};
  double coefficient_l2_norm{};
};

std::vector<std::string> split(const std::string &text, char delimiter) {
  std::vector<std::string> values;
  std::string cell;
  std::stringstream stream(text);
  while (std::getline(stream, cell, delimiter)) {
    values.push_back(cell);
  }
  return values;
}

std::vector<double> split_features(const std::string &text) {
  std::vector<double> values;
  for (const auto &cell : split(text, ';')) {
    values.push_back(cell.empty() ? 0.0 : std::stod(cell));
  }
  return values;
}

int64_t parse_int(const char *text, const char *name) {
  char *end = nullptr;
  const auto value = std::strtoll(text, &end, 10);
  if (end == text || *end != '\0' || value < 0) {
    throw std::runtime_error(std::string("invalid ") + name + ": " + text);
  }
  return value;
}

double parse_double(const char *text, const char *name) {
  char *end = nullptr;
  const auto value = std::strtod(text, &end);
  if (end == text || *end != '\0' || !std::isfinite(value)) {
    throw std::runtime_error(std::string("invalid ") + name + ": " + text);
  }
  return value;
}

std::string sha256_file(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("failed to open file for SHA-256: " +
                             path.string());
  }
  const std::string bytes((std::istreambuf_iterator<char>(input)),
                          std::istreambuf_iterator<char>());
  if (!input.good() && !input.eof()) {
    throw std::runtime_error("failed while reading file for SHA-256: " +
                             path.string());
  }
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

std::vector<double> parse_positive_double_list(const char *text,
                                               const char *name) {
  std::vector<double> values;
  for (const auto &cell : split(text, ',')) {
    if (cell.empty()) {
      throw std::runtime_error(std::string("empty value in ") + name);
    }
    const auto parsed = parse_double(cell.c_str(), name);
    if (!(parsed > 0.0)) {
      throw std::runtime_error(std::string(name) +
                               " values must be strictly positive");
    }
    values.push_back(parsed);
  }
  if (values.empty()) {
    throw std::runtime_error(std::string(name) + " must not be empty");
  }
  return values;
}

Options parse_options(int argc, char **argv) {
  Options opt;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    auto value = [&](const char *name) -> const char * {
      if (i + 1 >= argc) {
        throw std::runtime_error(std::string("missing value for ") + name);
      }
      return argv[++i];
    };
    if (arg == "--input") {
      opt.input_path = value("--input");
    } else if (arg == "--evaluation-input") {
      opt.evaluation_input_path = value("--evaluation-input");
    } else if (arg == "--output") {
      opt.output_path = value("--output");
    } else if (arg == "--affine-calibration-output") {
      opt.affine_calibration_output_path = value("--affine-calibration-output");
    } else if (arg == "--affine-calibration-metadata-output") {
      opt.affine_calibration_metadata_output_path =
          value("--affine-calibration-metadata-output");
    } else if (arg == "--affine-calibration-source-schema") {
      opt.affine_calibration_source_schema =
          value("--affine-calibration-source-schema");
    } else if (arg == "--affine-calibration-fit-probe-sha256") {
      opt.affine_calibration_fit_probe_sha256 =
          value("--affine-calibration-fit-probe-sha256");
    } else if (arg == "--affine-calibration-representation-checkpoint-sha256") {
      opt.affine_calibration_representation_checkpoint_sha256 =
          value("--affine-calibration-representation-checkpoint-sha256");
    } else if (arg == "--affine-calibration-mdn-checkpoint-sha256") {
      opt.affine_calibration_mdn_checkpoint_sha256 =
          value("--affine-calibration-mdn-checkpoint-sha256");
    } else if (arg == "--affine-calibration-graph-fingerprint") {
      opt.affine_calibration_graph_fingerprint =
          value("--affine-calibration-graph-fingerprint");
    } else if (arg == "--affine-calibration-capture-authority") {
      opt.affine_calibration_capture_authority =
          value("--affine-calibration-capture-authority");
    } else if (arg == "--schema-id") {
      opt.schema_id = value("--schema-id");
    } else if (arg == "--identity-modes") {
      opt.identity_modes = split(value("--identity-modes"), ',');
    } else if (arg == "--feature-dim") {
      opt.feature_dim = parse_int(value("--feature-dim"), "--feature-dim");
    } else if (arg == "--identity-embedding-dim") {
      opt.identity_embedding_dim = parse_int(value("--identity-embedding-dim"),
                                             "--identity-embedding-dim");
    } else if (arg == "--steps") {
      opt.steps = parse_int(value("--steps"), "--steps");
    } else if (arg == "--batch-size") {
      opt.batch_size = parse_int(value("--batch-size"), "--batch-size");
    } else if (arg == "--overfit-anchors") {
      opt.overfit_anchors =
          parse_int(value("--overfit-anchors"), "--overfit-anchors");
    } else if (arg == "--overfit-steps") {
      opt.overfit_steps =
          parse_int(value("--overfit-steps"), "--overfit-steps");
    } else if (arg == "--standardized-linear-steps") {
      opt.standardized_linear_steps = parse_int(
          value("--standardized-linear-steps"), "--standardized-linear-steps");
    } else if (arg == "--seed") {
      opt.seed = parse_int(value("--seed"), "--seed");
    } else if (arg == "--fit-fraction") {
      opt.fit_fraction =
          parse_double(value("--fit-fraction"), "--fit-fraction");
    } else if (arg == "--ridge-alphas") {
      opt.ridge_alphas =
          parse_positive_double_list(value("--ridge-alphas"), "--ridge-alphas");
    } else if (arg == "--ridge-validation-fraction") {
      opt.ridge_validation_fraction = parse_double(
          value("--ridge-validation-fraction"), "--ridge-validation-fraction");
    } else if (arg == "--ridge-validation-gap") {
      opt.ridge_validation_gap =
          parse_int(value("--ridge-validation-gap"), "--ridge-validation-gap");
    } else if (arg == "--ridge-direction-gate") {
      opt.ridge_direction_gate = parse_double(value("--ridge-direction-gate"),
                                              "--ridge-direction-gate");
    } else if (arg == "--ridge-rank-gate") {
      opt.ridge_rank_gate =
          parse_double(value("--ridge-rank-gate"), "--ridge-rank-gate");
    } else if (arg == "--learning-rate") {
      opt.learning_rate =
          parse_double(value("--learning-rate"), "--learning-rate");
    } else if (arg == "--grad-clip-norm") {
      opt.grad_clip_norm =
          parse_double(value("--grad-clip-norm"), "--grad-clip-norm");
    } else if (arg == "--regression-weight") {
      opt.regression_weight =
          parse_double(value("--regression-weight"), "--regression-weight");
    } else if (arg == "--direction-weight") {
      opt.direction_weight =
          parse_double(value("--direction-weight"), "--direction-weight");
    } else if (arg == "--rank-weight") {
      opt.rank_weight = parse_double(value("--rank-weight"), "--rank-weight");
    } else if (arg == "--huber-beta") {
      opt.huber_beta = parse_double(value("--huber-beta"), "--huber-beta");
    } else if (arg == "--logit-scale") {
      opt.logit_scale = parse_double(value("--logit-scale"), "--logit-scale");
    } else if (arg == "--target-scale") {
      opt.target_scale =
          parse_double(value("--target-scale"), "--target-scale");
    } else if (arg == "--margin-eps") {
      opt.margin_eps = parse_double(value("--margin-eps"), "--margin-eps");
    } else if (arg == "--rank-margin-eps") {
      opt.rank_margin_eps =
          parse_double(value("--rank-margin-eps"), "--rank-margin-eps");
    } else if (arg == "--cpu") {
      opt.force_cpu = true;
    } else if (arg == "--ridge-only") {
      opt.ridge_only = true;
    } else if (arg == "--affine-parity-only") {
      opt.affine_parity_only = true;
    } else {
      throw std::runtime_error("unknown argument: " + arg);
    }
  }

  if (opt.input_path.empty() || opt.output_path.empty()) {
    throw std::runtime_error("--input and --output are required");
  }
  if (opt.feature_dim <= 0 || opt.steps <= 0 || opt.batch_size <= 0 ||
      opt.overfit_steps <= 0 || opt.standardized_linear_steps <= 0 ||
      !(opt.fit_fraction > 0.0) || !(opt.fit_fraction < 1.0) ||
      !(opt.ridge_validation_fraction > 0.0) ||
      !(opt.ridge_validation_fraction < 1.0) || opt.ridge_alphas.empty() ||
      !(opt.ridge_direction_gate > 0.0) || !(opt.ridge_direction_gate <= 1.0) ||
      !(opt.ridge_rank_gate > 0.0) || !(opt.ridge_rank_gate <= 1.0) ||
      !(opt.learning_rate > 0.0) || !(opt.huber_beta > 0.0) ||
      !(opt.logit_scale > 0.0) || !(opt.target_scale > 0.0)) {
    throw std::runtime_error("invalid non-positive parity option");
  }
  if (opt.identity_modes.empty()) {
    throw std::runtime_error("--identity-modes must not be empty");
  }
  const bool calibration_requested =
      !opt.affine_calibration_output_path.empty() ||
      !opt.affine_calibration_metadata_output_path.empty() ||
      !opt.affine_calibration_source_schema.empty() ||
      !opt.affine_calibration_fit_probe_sha256.empty() ||
      !opt.affine_calibration_representation_checkpoint_sha256.empty() ||
      !opt.affine_calibration_mdn_checkpoint_sha256.empty() ||
      !opt.affine_calibration_graph_fingerprint.empty() ||
      !opt.affine_calibration_capture_authority.empty();
  if (calibration_requested &&
      (!opt.affine_parity_only || opt.affine_calibration_output_path.empty() ||
       opt.affine_calibration_metadata_output_path.empty() ||
       opt.affine_calibration_source_schema.empty() ||
       opt.affine_calibration_fit_probe_sha256.empty() ||
       opt.affine_calibration_representation_checkpoint_sha256.empty() ||
       opt.affine_calibration_mdn_checkpoint_sha256.empty() ||
       opt.affine_calibration_graph_fingerprint.empty() ||
       opt.affine_calibration_capture_authority.empty())) {
    throw std::runtime_error(
        "affine calibration export requires --affine-parity-only and all "
        "affine calibration output/provenance options");
  }
  if (calibration_requested) {
    const auto normalized = [](const std::filesystem::path &path) {
      std::error_code error;
      const auto absolute = std::filesystem::absolute(path, error);
      if (error) {
        throw std::runtime_error("failed to normalize output path: " +
                                 path.string());
      }
      return absolute.lexically_normal();
    };
    const std::vector<std::filesystem::path> outputs{
        normalized(opt.output_path),
        normalized(opt.affine_calibration_output_path),
        normalized(opt.affine_calibration_metadata_output_path),
    };
    if (std::set<std::filesystem::path>(outputs.begin(), outputs.end())
            .size() != outputs.size()) {
      throw std::runtime_error(
          "result, affine archive, and affine metadata paths must be distinct");
    }
    const auto input = normalized(opt.input_path);
    const auto evaluation = opt.evaluation_input_path.empty()
                                ? std::filesystem::path{}
                                : normalized(opt.evaluation_input_path);
    if (std::find(outputs.begin(), outputs.end(), input) != outputs.end() ||
        (!evaluation.empty() && std::find(outputs.begin(), outputs.end(),
                                          evaluation) != outputs.end())) {
      throw std::runtime_error(
          "affine calibration outputs must not alias an input probe");
    }
  }
  const std::set<std::string> allowed{"shared", "edge_embedding", "per_edge",
                                      "edge_embedding_per_edge"};
  for (const auto &mode : opt.identity_modes) {
    if (!allowed.contains(mode)) {
      throw std::runtime_error("invalid identity mode: " + mode);
    }
  }
  return opt;
}

Dataset read_dataset(const Options &opt) {
  std::vector<Row> rows;
  std::set<int> anchor_set;
  std::set<int> primary_anchor_set;
  int max_edge = -1;
  int max_channel = -1;
  int64_t source_feature_count = -1;
  std::vector<std::filesystem::path> input_paths{opt.input_path};
  if (!opt.evaluation_input_path.empty()) {
    input_paths.push_back(opt.evaluation_input_path);
  }
  for (std::size_t path_index = 0; path_index < input_paths.size();
       ++path_index) {
    std::ifstream input(input_paths[path_index]);
    if (!input) {
      throw std::runtime_error("failed to open input probe: " +
                               input_paths[path_index].string());
    }
    std::string line;
    bool header = true;
    while (std::getline(input, line)) {
      if (line.empty()) {
        continue;
      }
      if (header) {
        header = false;
        continue;
      }
      const auto columns = split(line, ',');
      if (columns.size() < 12) {
        throw std::runtime_error("probe row has fewer than 12 columns");
      }
      Row row;
      row.anchor_index = std::stoi(columns[2]);
      row.edge_index = std::stoi(columns[4]);
      row.edge_id = columns[5];
      row.base_node_id = columns[6];
      row.quote_node_id = columns[7];
      row.channel_index = std::stoi(columns[8]);
      row.target = std::stod(columns[9]);
      row.features = split_features(columns[11]);
      if (row.edge_id.empty() || row.base_node_id.empty() ||
          row.quote_node_id.empty() || !std::isfinite(row.target) ||
          std::any_of(row.features.begin(), row.features.end(),
                      [](double value) { return !std::isfinite(value); })) {
        throw std::runtime_error(
            "probe row contains empty identity or non-finite numeric data");
      }
      const auto declared_count = std::stoll(columns[10]);
      if (declared_count != static_cast<int64_t>(row.features.size())) {
        throw std::runtime_error("declared feature count does not match row");
      }
      if (source_feature_count < 0) {
        source_feature_count = declared_count;
      } else if (source_feature_count != declared_count) {
        throw std::runtime_error("inconsistent feature count in input probe");
      }
      anchor_set.insert(row.anchor_index);
      if (path_index == 0) {
        primary_anchor_set.insert(row.anchor_index);
      }
      max_edge = std::max(max_edge, row.edge_index);
      max_channel = std::max(max_channel, row.channel_index);
      rows.push_back(std::move(row));
    }
  }
  if (rows.empty() || anchor_set.size() < 2 || max_edge < 0 ||
      max_channel < 0) {
    throw std::runtime_error("input probe has insufficient usable rows");
  }

  const int64_t H = opt.feature_dim;
  const int64_t E = max_edge + 1;
  const int64_t C = max_channel + 1;
  const int64_t N = E + 1;
  const int64_t dynamic_feature_count = 3 * H;
  if (source_feature_count < dynamic_feature_count) {
    throw std::runtime_error("probe does not contain 3H dynamic features");
  }

  Dataset dataset;
  dataset.anchors.assign(anchor_set.begin(), anchor_set.end());
  dataset.edge_count = E;
  dataset.channel_count = C;
  dataset.feature_dim = H;
  dataset.source_feature_count = source_feature_count;
  dataset.edge_ids.resize(static_cast<std::size_t>(E));
  dataset.base_node_ids.resize(static_cast<std::size_t>(E));
  dataset.primary_anchor_count =
      static_cast<int64_t>(primary_anchor_set.size());
  const int64_t A = static_cast<int64_t>(dataset.anchors.size());
  std::map<int, int64_t> anchor_to_index;
  for (int64_t i = 0; i < A; ++i) {
    anchor_to_index[dataset.anchors[static_cast<std::size_t>(i)]] = i;
  }

  const auto nan = std::numeric_limits<float>::quiet_NaN();
  std::vector<float> context_values(static_cast<std::size_t>(A * N * C * H),
                                    nan);
  std::vector<float> readout_values(
      static_cast<std::size_t>(A * E * C * source_feature_count), nan);
  std::vector<float> target_values(static_cast<std::size_t>(A * E * C), nan);
  std::vector<bool> edge_seen(static_cast<std::size_t>(A * E * C), false);
  std::vector<bool> quote_seen(static_cast<std::size_t>(A * C), false);
  std::vector<bool> edge_identity_seen(static_cast<std::size_t>(E), false);

  auto context_offset = [=](int64_t a, int64_t n, int64_t c, int64_t h) {
    return static_cast<std::size_t>(((a * N + n) * C + c) * H + h);
  };
  auto target_offset = [=](int64_t a, int64_t e, int64_t c) {
    return static_cast<std::size_t>((a * E + e) * C + c);
  };
  auto readout_offset = [=](int64_t a, int64_t e, int64_t c, int64_t f) {
    return static_cast<std::size_t>(
        (((a * E + e) * C + c) * source_feature_count) + f);
  };
  auto quote_seen_offset = [=](int64_t a, int64_t c) {
    return static_cast<std::size_t>(a * C + c);
  };

  for (const auto &row : rows) {
    const int64_t a = anchor_to_index.at(row.anchor_index);
    const int64_t e = row.edge_index;
    const int64_t c = row.channel_index;
    if (e < 0 || e >= E || c < 0 || c >= C) {
      throw std::runtime_error("negative or out-of-range probe coordinate");
    }
    const auto seen_index = target_offset(a, e, c);
    if (edge_seen[seen_index]) {
      throw std::runtime_error("duplicate anchor/edge/channel probe row");
    }
    const auto edge_slot = static_cast<std::size_t>(e);
    if (!edge_identity_seen[edge_slot]) {
      dataset.edge_ids[edge_slot] = row.edge_id;
      dataset.base_node_ids[edge_slot] = row.base_node_id;
      edge_identity_seen[edge_slot] = true;
    } else if (dataset.edge_ids[edge_slot] != row.edge_id ||
               dataset.base_node_ids[edge_slot] != row.base_node_id) {
      throw std::runtime_error(
          "probe edge identity is inconsistent across rows");
    }
    if (dataset.quote_node_id.empty()) {
      dataset.quote_node_id = row.quote_node_id;
    } else if (dataset.quote_node_id != row.quote_node_id) {
      throw std::runtime_error(
          "probe quote node identity is inconsistent across rows");
    }
    edge_seen[seen_index] = true;
    target_values[seen_index] = static_cast<float>(row.target);
    for (int64_t feature = 0; feature < source_feature_count; ++feature) {
      readout_values[readout_offset(a, e, c, feature)] =
          static_cast<float>(row.features[static_cast<std::size_t>(feature)]);
    }
    const auto quote_index = quote_seen_offset(a, c);
    for (int64_t h = 0; h < H; ++h) {
      const double base = row.features[static_cast<std::size_t>(h)];
      const double quote = row.features[static_cast<std::size_t>(H + h)];
      const double difference =
          row.features[static_cast<std::size_t>(2 * H + h)];
      dataset.reconstruction_max_abs_error =
          std::max(dataset.reconstruction_max_abs_error,
                   std::fabs((base - quote) - difference));
      context_values[context_offset(a, e + 1, c, h)] = static_cast<float>(base);
      const auto qo = context_offset(a, 0, c, h);
      if (!quote_seen[quote_index]) {
        context_values[qo] = static_cast<float>(quote);
      } else {
        dataset.quote_consistency_max_abs_error = std::max(
            dataset.quote_consistency_max_abs_error,
            std::fabs(static_cast<double>(context_values[qo]) - quote));
      }
    }
    quote_seen[quote_index] = true;
  }

  if (std::find(edge_seen.begin(), edge_seen.end(), false) != edge_seen.end() ||
      std::find(quote_seen.begin(), quote_seen.end(), false) !=
          quote_seen.end()) {
    throw std::runtime_error("probe is missing complete anchor groups");
  }
  for (const auto value : context_values) {
    if (!std::isfinite(value)) {
      throw std::runtime_error(
          "reconstructed context contains non-finite data");
    }
  }
  for (const auto value : target_values) {
    if (!std::isfinite(value)) {
      throw std::runtime_error("target contains non-finite data");
    }
  }
  for (const auto value : readout_values) {
    if (!std::isfinite(value)) {
      throw std::runtime_error(
          "cached readout features contain non-finite data");
    }
  }
  if (std::find(edge_identity_seen.begin(), edge_identity_seen.end(), false) !=
          edge_identity_seen.end() ||
      std::set<std::string>(dataset.edge_ids.begin(), dataset.edge_ids.end())
              .size() != static_cast<std::size_t>(E)) {
    throw std::runtime_error(
        "probe edge identities are incomplete or duplicated");
  }

  dataset.context =
      torch::from_blob(context_values.data(), {A, N, C, H}, torch::kFloat32)
          .clone();
  dataset.readout_features =
      torch::from_blob(readout_values.data(), {A, E, C, source_feature_count},
                       torch::kFloat32)
          .clone();
  dataset.target =
      torch::from_blob(target_values.data(), {A, E, C}, torch::kFloat32)
          .clone();
  return dataset;
}

Objective compute_objective(const torch::Tensor &prediction,
                            const torch::Tensor &target, const Options &opt) {
  const auto zero = prediction.new_zeros({});
  const auto predicted_scaled = prediction * opt.target_scale;
  const auto target_scaled = target * opt.target_scale;
  const auto diff = predicted_scaled - target_scaled;
  const auto abs_diff = diff.abs();
  const auto regression = torch::where(abs_diff < opt.huber_beta,
                                       0.5 * diff.pow(2) / opt.huber_beta,
                                       abs_diff - 0.5 * opt.huber_beta)
                              .mean();

  auto direction = zero;
  const auto target_sign = target.sign();
  const auto direction_active = target_sign.ne(0);
  if (direction_active.sum().item<int64_t>() > 0) {
    direction =
        torch::softplus(-(predicted_scaled.masked_select(direction_active) *
                          target_sign.masked_select(direction_active) *
                          opt.logit_scale))
            .mean();
  }

  auto rank_sum = zero;
  int64_t rank_count = 0;
  for (int64_t lhs = 0; lhs < prediction.size(1); ++lhs) {
    for (int64_t rhs = lhs + 1; rhs < prediction.size(1); ++rhs) {
      const auto predicted_difference =
          (prediction.select(1, lhs) - prediction.select(1, rhs)) *
          opt.target_scale;
      const auto rank_sign =
          (target.select(1, lhs) - target.select(1, rhs)).sign();
      const auto active = rank_sign.ne(0);
      const auto count = active.sum().item<int64_t>();
      if (count <= 0) {
        continue;
      }
      rank_sum =
          rank_sum +
          torch::softplus(-(predicted_difference.masked_select(active) *
                            rank_sign.masked_select(active) * opt.logit_scale))
              .sum();
      rank_count += count;
    }
  }
  const auto rank =
      rank_count > 0 ? rank_sum / static_cast<double>(rank_count) : zero;
  const auto total = opt.regression_weight * regression +
                     opt.direction_weight * direction + opt.rank_weight * rank;
  return {total, regression, direction, rank};
}

mdn::DirectEdgeReturnHead make_head(const Dataset &dataset, const Options &opt,
                                    const std::string &identity_mode,
                                    const torch::Device &device,
                                    int64_t seed_offset) {
  torch::manual_seed(opt.seed + seed_offset);
  if (device.is_cuda()) {
    torch::cuda::manual_seed_all(opt.seed + seed_offset);
  }
  const bool uses_embedding = identity_mode == "edge_embedding" ||
                              identity_mode == "edge_embedding_per_edge";
  auto head = mdn::DirectEdgeReturnHead(mdn::DirectEdgeReturnHeadOptions{
      .feature_dim = dataset.feature_dim,
      .quote_node_index = 0,
      .identity_mode = identity_mode,
      .base_edge_count = dataset.edge_count,
      .identity_embedding_dim = uses_embedding ? opt.identity_embedding_dim : 0,
      // The probe is captured after the Runtime adapter. Reapplying an
      // adapter here would test a different boundary.
      .adapter_hidden_dim = 0,
  });
  head->to(device);
  return head;
}

torch::Tensor index_tensor(const std::vector<int64_t> &indices,
                           const torch::Device &device) {
  return torch::tensor(indices, torch::TensorOptions().dtype(torch::kInt64))
      .to(device);
}

StandardizedPerEdgeLinearHead
make_standardized_linear_head(const Dataset &dataset,
                              const std::vector<int64_t> &fit_indices,
                              const torch::Device &device, const Options &opt) {
  torch::manual_seed(opt.seed + 2000);
  if (device.is_cuda()) {
    torch::cuda::manual_seed_all(opt.seed + 2000);
  }
  const auto indices = index_tensor(fit_indices, torch::Device(torch::kCPU));
  const auto fit_context = dataset.context.index_select(0, indices);
  const auto B = fit_context.size(0);
  const auto C = fit_context.size(2);
  const auto E = dataset.edge_count;
  const auto H = dataset.feature_dim;
  auto quote = fit_context.select(1, 0).unsqueeze(1).expand({B, E, C, H});
  auto base = fit_context.narrow(1, 1, E);
  auto flat = torch::cat({base, quote, base - quote}, /*dim=*/-1)
                  .contiguous()
                  .view({B * E * C, 3 * H});
  const auto mean = flat.mean(/*dim=*/0);
  const auto variance = (flat - mean).pow(2).mean(/*dim=*/0);
  const auto standard_deviation = variance.sqrt();
  const auto inv_std = torch::where(standard_deviation > 1.0e-12,
                                    standard_deviation.reciprocal(),
                                    torch::ones_like(standard_deviation));
  auto head = StandardizedPerEdgeLinearHead(H, E, mean, inv_std);
  head->to(device);
  return head;
}

AnalyticRidgeFit
fit_analytic_per_edge_ridge(const Dataset &dataset,
                            const std::vector<int64_t> &fit_indices,
                            double alpha) {
  if (fit_indices.empty() || !(alpha > 0.0)) {
    throw std::runtime_error(
        "analytic ridge requires fit anchors and a positive alpha");
  }
  torch::NoGradGuard no_grad;
  const auto indices = index_tensor(fit_indices, torch::Device(torch::kCPU));
  const auto fit_context =
      dataset.context.index_select(0, indices).to(torch::kFloat64);
  const auto fit_target =
      dataset.target.index_select(0, indices).to(torch::kFloat64);
  const auto B = fit_context.size(0);
  const auto C = fit_context.size(2);
  const auto E = dataset.edge_count;
  const auto H = dataset.feature_dim;
  const auto F = 3 * H;
  auto quote = fit_context.select(1, 0).unsqueeze(1).expand({B, E, C, H});
  auto base = fit_context.narrow(1, 1, E);
  auto features =
      torch::cat({base, quote, base - quote}, /*dim=*/-1).contiguous();
  auto flat = features.view({B * E * C, F});
  const auto feature_mean = flat.mean(/*dim=*/0);
  const auto variance = (flat - feature_mean).pow(2).mean(/*dim=*/0);
  const auto standard_deviation = variance.sqrt();
  const auto feature_inv_std = torch::where(
      standard_deviation > 1.0e-12, standard_deviation.reciprocal(),
      torch::ones_like(standard_deviation));
  const auto standardized = (features - feature_mean.view({1, 1, 1, F})) *
                            feature_inv_std.view({1, 1, 1, F});

  auto weights = torch::zeros({E, F}, torch::kFloat64);
  auto bias = torch::zeros({E}, torch::kFloat64);
  double max_normalized_residual = 0.0;
  for (int64_t edge = 0; edge < E; ++edge) {
    const auto x = standardized.select(1, edge).contiguous().view({B * C, F});
    const auto y = fit_target.select(1, edge).contiguous().view({B * C});
    const auto x_mean = x.mean(/*dim=*/0);
    const auto y_mean = y.mean();
    const auto centered_x = x - x_mean;
    const auto centered_y = y - y_mean;
    auto gram = centered_x.transpose(0, 1).matmul(centered_x);
    gram.diagonal(/*offset=*/0, /*dim1=*/0, /*dim2=*/1)
        .add_(static_cast<double>(x.size(0)) * alpha);
    const auto rhs = centered_x.transpose(0, 1).matmul(centered_y.unsqueeze(1));
    auto [cholesky, info] = at::linalg_cholesky_ex(gram, /*upper=*/false,
                                                   /*check_errors=*/false);
    const auto cholesky_info = info.item<int64_t>();
    if (cholesky_info != 0) {
      throw std::runtime_error("ridge Cholesky failed for edge " +
                               std::to_string(edge) +
                               " with info=" + std::to_string(cholesky_info));
    }
    const auto edge_weights =
        at::cholesky_solve(rhs, cholesky, /*upper=*/false).squeeze(1);
    if (!torch::isfinite(edge_weights).all().item<bool>()) {
      throw std::runtime_error("ridge produced non-finite coefficients");
    }
    const auto residual = gram.matmul(edge_weights.unsqueeze(1)) - rhs;
    const auto normalized_residual =
        residual.norm().item<double>() /
        std::max(rhs.norm().item<double>(), 1.0e-30);
    if (!std::isfinite(normalized_residual) || normalized_residual > 1.0e-7) {
      throw std::runtime_error("ridge solve residual exceeded tolerance");
    }
    max_normalized_residual =
        std::max(max_normalized_residual, normalized_residual);
    weights.select(0, edge).copy_(edge_weights);
    bias[edge] = y_mean - x_mean.dot(edge_weights);
  }
  auto head =
      AnalyticRidgeHead(H, E, feature_mean, feature_inv_std, weights, bias);
  return {head, max_normalized_residual, weights.norm().item<double>()};
}

template <typename HeadT>
TrainSummary train_head(HeadT &head, const torch::Tensor &context,
                        const torch::Tensor &target,
                        const std::vector<int64_t> &training_indices,
                        int64_t steps, const Options &opt, int64_t seed) {
  if (training_indices.empty()) {
    throw std::runtime_error("cannot train on an empty index set");
  }
  head->train();
  torch::optim::Adam optimizer(head->parameters(),
                               torch::optim::AdamOptions(opt.learning_rate));
  std::vector<int64_t> order = training_indices;
  std::mt19937_64 random(static_cast<std::mt19937_64::result_type>(seed));
  std::shuffle(order.begin(), order.end(), random);
  std::size_t cursor = 0;
  TrainSummary summary;
  for (int64_t step = 0; step < steps; ++step) {
    if (cursor >= order.size()) {
      std::shuffle(order.begin(), order.end(), random);
      cursor = 0;
    }
    const auto end = std::min(
        order.size(), cursor + static_cast<std::size_t>(opt.batch_size));
    std::vector<int64_t> batch_indices(
        order.begin() + static_cast<std::ptrdiff_t>(cursor),
        order.begin() + static_cast<std::ptrdiff_t>(end));
    cursor = end;
    const auto indices = index_tensor(batch_indices, context.device());
    const auto batch_context = context.index_select(0, indices);
    const auto batch_target = target.index_select(0, indices);
    optimizer.zero_grad();
    const auto prediction = head->forward(batch_context);
    const auto objective = compute_objective(prediction, batch_target, opt);
    if (!torch::isfinite(objective.total).template item<bool>()) {
      throw std::runtime_error("non-finite cached-feature parity loss");
    }
    objective.total.backward();
    if (opt.grad_clip_norm > 0.0) {
      torch::nn::utils::clip_grad_norm_(head->parameters(), opt.grad_clip_norm);
    }
    optimizer.step();
    const auto loss = objective.total.detach().template item<double>();
    if (step == 0) {
      summary.initial_loss = loss;
    }
    summary.final_loss = loss;
  }
  return summary;
}

int sign(double value) {
  if (value > 0.0) {
    return 1;
  }
  if (value < 0.0) {
    return -1;
  }
  return 0;
}

template <typename HeadT>
Metric evaluate_head(HeadT &head, const torch::Tensor &context,
                     const torch::Tensor &target,
                     const std::vector<int64_t> &indices, const Options &opt) {
  Metric metric;
  if (indices.empty()) {
    return metric;
  }
  head->eval();
  torch::NoGradGuard no_grad;
  const auto selected = index_tensor(indices, context.device());
  const auto prediction =
      head->forward(context.index_select(0, selected)).to(torch::kCPU);
  const auto realized = target.index_select(0, selected).to(torch::kCPU);
  const auto p = prediction.template accessor<float, 3>();
  const auto t = realized.template accessor<float, 3>();
  for (int64_t b = 0; b < prediction.size(0); ++b) {
    for (int64_t c = 0; c < prediction.size(2); ++c) {
      int best_prediction = 0;
      int best_target = 0;
      for (int64_t e = 0; e < prediction.size(1); ++e) {
        const double predicted = p[b][e][c];
        const double actual = t[b][e][c];
        ++metric.count;
        metric.abs_error += std::fabs(predicted - actual);
        metric.sq_error += (predicted - actual) * (predicted - actual);
        metric.pred_sum += predicted;
        metric.target_sum += actual;
        metric.pred_sq_sum += predicted * predicted;
        metric.target_sq_sum += actual * actual;
        metric.cross_sum += predicted * actual;
        if (sign(predicted) == sign(actual)) {
          ++metric.sign_correct;
        }
        if (std::fabs(actual) > opt.margin_eps) {
          ++metric.margin_count;
          if (sign(predicted) == sign(actual)) {
            ++metric.margin_sign_correct;
          }
        }
        if (p[b][e][c] > p[b][best_prediction][c]) {
          best_prediction = static_cast<int>(e);
        }
        if (t[b][e][c] > t[b][best_target][c]) {
          best_target = static_cast<int>(e);
        }
      }
      ++metric.best_count;
      if (best_prediction == best_target) {
        ++metric.best_correct;
      }
      for (int64_t lhs = 0; lhs < prediction.size(1); ++lhs) {
        for (int64_t rhs = lhs + 1; rhs < prediction.size(1); ++rhs) {
          const double predicted_difference = p[b][lhs][c] - p[b][rhs][c];
          const double target_difference = t[b][lhs][c] - t[b][rhs][c];
          ++metric.pair_count;
          if (sign(predicted_difference) == sign(target_difference)) {
            ++metric.pair_correct;
          }
          if (std::fabs(target_difference) > opt.rank_margin_eps) {
            ++metric.margin_pair_count;
            if (sign(predicted_difference) == sign(target_difference)) {
              ++metric.margin_pair_correct;
            }
          }
        }
      }
    }
  }
  return metric;
}

double correlation(const Metric &metric) {
  if (metric.count <= 1) {
    return 0.0;
  }
  const auto n = static_cast<double>(metric.count);
  const auto numerator =
      metric.cross_sum - metric.pred_sum * metric.target_sum / n;
  const auto left = metric.pred_sq_sum - metric.pred_sum * metric.pred_sum / n;
  const auto right =
      metric.target_sq_sum - metric.target_sum * metric.target_sum / n;
  if (left <= 0.0 || right <= 0.0) {
    return 0.0;
  }
  return numerator / std::sqrt(left * right);
}

double standard_deviation(double sum, double square_sum, int64_t count) {
  if (count <= 1) {
    return 0.0;
  }
  const auto n = static_cast<double>(count);
  return std::sqrt(std::max(0.0, square_sum / n - (sum / n) * (sum / n)));
}

void emit_metric(std::ostream &out, const std::string &prefix,
                 const Metric &metric) {
  out << prefix << ".count=" << metric.count << '\n';
  if (metric.count > 0) {
    const auto n = static_cast<double>(metric.count);
    const auto prediction_std =
        standard_deviation(metric.pred_sum, metric.pred_sq_sum, metric.count);
    const auto target_std = standard_deviation(
        metric.target_sum, metric.target_sq_sum, metric.count);
    out << prefix << ".mae=" << metric.abs_error / n << '\n';
    out << prefix << ".rmse=" << std::sqrt(metric.sq_error / n) << '\n';
    out << prefix << ".directional_accuracy="
        << static_cast<double>(metric.sign_correct) / n << '\n';
    out << prefix << ".correlation=" << correlation(metric) << '\n';
    out << prefix << ".pred_std=" << prediction_std << '\n';
    out << prefix << ".target_std=" << target_std << '\n';
    out << prefix << ".pred_to_target_std_ratio="
        << (target_std > 0.0 ? prediction_std / target_std : 0.0) << '\n';
  }
  out << prefix << ".margin_count=" << metric.margin_count << '\n';
  if (metric.margin_count > 0) {
    out << prefix << ".margin_directional_accuracy="
        << static_cast<double>(metric.margin_sign_correct) /
               static_cast<double>(metric.margin_count)
        << '\n';
  }
  out << prefix << ".pairwise_rank_count=" << metric.pair_count << '\n';
  if (metric.pair_count > 0) {
    out << prefix << ".pairwise_rank_accuracy="
        << static_cast<double>(metric.pair_correct) /
               static_cast<double>(metric.pair_count)
        << '\n';
  }
  out << prefix << ".margin_pairwise_rank_count=" << metric.margin_pair_count
      << '\n';
  if (metric.margin_pair_count > 0) {
    out << prefix << ".margin_pairwise_rank_accuracy="
        << static_cast<double>(metric.margin_pair_correct) /
               static_cast<double>(metric.margin_pair_count)
        << '\n';
  }
  out << prefix << ".best_asset_count=" << metric.best_count << '\n';
  if (metric.best_count > 0) {
    out << prefix << ".best_asset_agreement="
        << static_cast<double>(metric.best_correct) /
               static_cast<double>(metric.best_count)
        << '\n';
  }
}

std::vector<int64_t> integer_range(int64_t begin, int64_t end) {
  std::vector<int64_t> values(static_cast<std::size_t>(end - begin));
  std::iota(values.begin(), values.end(), begin);
  return values;
}

std::string bool_text(bool value) { return value ? "true" : "false"; }

double metric_rmse(const Metric &metric) {
  return metric.count > 0
             ? std::sqrt(metric.sq_error / static_cast<double>(metric.count))
             : std::numeric_limits<double>::infinity();
}

double metric_directional_accuracy(const Metric &metric) {
  return metric.count > 0 ? static_cast<double>(metric.sign_correct) /
                                static_cast<double>(metric.count)
                          : 0.0;
}

double metric_pairwise_rank_accuracy(const Metric &metric) {
  return metric.pair_count > 0 ? static_cast<double>(metric.pair_correct) /
                                     static_cast<double>(metric.pair_count)
                               : 0.0;
}

StandardizedPerEdgeLinearHead
make_registered_affine_head(const AnalyticRidgeHead &analytic,
                            int64_t feature_dim, int64_t edge_count) {
  const auto feature_count = 3 * feature_dim;
  auto registered = StandardizedPerEdgeLinearHead(
      feature_dim, edge_count, analytic->feature_mean.reshape({feature_count}),
      analytic->feature_inv_std.reshape({feature_count}));
  registered->to(torch::Device(torch::kCPU), torch::kFloat64);

  torch::NoGradGuard no_grad;
  const auto weights = analytic->weights.reshape({edge_count, feature_count});
  const auto bias = analytic->bias.reshape({edge_count});
  for (int64_t edge = 0; edge < edge_count; ++edge) {
    auto &projection =
        registered->edge_projections[static_cast<std::size_t>(edge)];
    projection->weight.copy_(weights.select(0, edge).view({1, feature_count}));
    projection->bias.copy_(bias.select(0, edge).view({1}));
  }
  return registered;
}

struct AffinePredictionParity {
  bool torch_equal{false};
  int64_t prediction_count{0};
  double max_abs_delta{std::numeric_limits<double>::infinity()};
  double mean_abs_delta{std::numeric_limits<double>::infinity()};
};

void append_u32_be(std::string &bytes, std::uint32_t value) {
  for (int shift = 24; shift >= 0; shift -= 8) {
    bytes.push_back(static_cast<char>((value >> shift) & 0xffU));
  }
}

std::string canonical_float32_tensor_digest(const torch::Tensor &tensor,
                                            const std::string &domain) {
  TORCH_CHECK(tensor.defined() && tensor.device().is_cpu() &&
                  tensor.scalar_type() == torch::kFloat32 &&
                  torch::isfinite(tensor).all().item<bool>() && !domain.empty(),
              "tensor digest requires a domain and finite CPU float32 data");
  const auto contiguous = tensor.contiguous();
  std::string bytes = domain + "\n";
  bytes += "dtype=float32\n";
  bytes += "rank=" + std::to_string(contiguous.dim()) + "\n";
  for (int64_t dim = 0; dim < contiguous.dim(); ++dim) {
    bytes += "shape." + std::to_string(dim) + "=" +
             std::to_string(contiguous.size(dim)) + "\n";
  }
  bytes += "encoding=ieee754_binary32_big_endian.v1\n";
  const auto *values = contiguous.data_ptr<float>();
  for (int64_t i = 0; i < contiguous.numel(); ++i) {
    append_u32_be(bytes, std::bit_cast<std::uint32_t>(values[i]));
  }
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

std::string canonical_prediction_digest(const torch::Tensor &prediction) {
  return canonical_float32_tensor_digest(prediction,
                                         "mdn_affine_prediction_tensor.v1");
}

AffinePredictionParity compare_prediction_tensors(const torch::Tensor &lhs,
                                                  const torch::Tensor &rhs) {
  TORCH_CHECK(lhs.defined() && rhs.defined() && lhs.sizes() == rhs.sizes(),
              "prediction tensor parity shape mismatch");
  const auto lhs64 = lhs.to(torch::kCPU).to(torch::kFloat64);
  const auto rhs64 = rhs.to(torch::kCPU).to(torch::kFloat64);
  const auto delta = (lhs64 - rhs64).abs();
  return {
      .torch_equal = lhs.equal(rhs),
      .prediction_count = lhs.numel(),
      .max_abs_delta = delta.max().item<double>(),
      .mean_abs_delta = delta.mean().item<double>(),
  };
}

void emit_prediction_tensor_parity(std::ostream &out, const std::string &prefix,
                                   const torch::Tensor &lhs,
                                   const torch::Tensor &rhs) {
  const auto parity = compare_prediction_tensors(lhs, rhs);
  out << prefix << ".prediction_count=" << parity.prediction_count << '\n';
  out << prefix << ".torch_equal=" << bool_text(parity.torch_equal) << '\n';
  out << std::scientific << std::setprecision(17);
  out << prefix << ".max_abs_delta=" << parity.max_abs_delta << '\n';
  out << prefix << ".mean_abs_delta=" << parity.mean_abs_delta << '\n';
  out << std::fixed << std::setprecision(12);
}

AffinePredictionParity compare_affine_predictions(
    AnalyticRidgeHead &analytic, StandardizedPerEdgeLinearHead &registered,
    const torch::Tensor &context, const std::vector<int64_t> &indices) {
  if (indices.empty()) {
    throw std::runtime_error("affine parity split must not be empty");
  }
  analytic->eval();
  registered->eval();
  torch::NoGradGuard no_grad;
  const auto selected = index_tensor(indices, torch::Device(torch::kCPU));
  const auto selected_context = context.index_select(0, selected);
  const auto analytic_prediction = analytic->forward(selected_context);
  const auto registered_prediction = registered->forward(selected_context);
  TORCH_CHECK(analytic_prediction.sizes() == registered_prediction.sizes(),
              "affine parity prediction shape mismatch");
  const auto delta = (analytic_prediction.to(torch::kFloat64) -
                      registered_prediction.to(torch::kFloat64))
                         .abs();
  return {
      .torch_equal = analytic_prediction.equal(registered_prediction),
      .prediction_count = analytic_prediction.numel(),
      .max_abs_delta = delta.max().item<double>(),
      .mean_abs_delta = delta.mean().item<double>(),
  };
}

bool emit_affine_parity_split(
    std::ostream &out, const std::string &split, AnalyticRidgeHead &analytic,
    StandardizedPerEdgeLinearHead &registered, const torch::Tensor &context,
    const torch::Tensor &target, const std::vector<int64_t> &indices,
    const Metric &analytic_metric, const Options &opt) {
  constexpr double max_prediction_delta_tolerance = 1.0e-6;
  constexpr double metric_delta_tolerance = 1.0e-9;
  const auto prediction =
      compare_affine_predictions(analytic, registered, context, indices);
  const auto registered_metric =
      evaluate_head(registered, context, target, indices, opt);
  const auto direction_delta = metric_directional_accuracy(registered_metric) -
                               metric_directional_accuracy(analytic_metric);
  const auto rank_delta = metric_pairwise_rank_accuracy(registered_metric) -
                          metric_pairwise_rank_accuracy(analytic_metric);
  const auto rmse_delta =
      metric_rmse(registered_metric) - metric_rmse(analytic_metric);
  const auto prefix = "affine_parity." + split;
  out << prefix << ".prediction_count=" << prediction.prediction_count << '\n';
  out << prefix << ".torch_equal=" << bool_text(prediction.torch_equal) << '\n';
  out << std::scientific << std::setprecision(17);
  out << prefix << ".max_abs_delta=" << prediction.max_abs_delta << '\n';
  out << prefix << ".mean_abs_delta=" << prediction.mean_abs_delta << '\n';
  out << prefix << ".directional_accuracy_delta=" << direction_delta << '\n';
  out << prefix << ".pairwise_rank_accuracy_delta=" << rank_delta << '\n';
  out << prefix << ".rmse_delta=" << rmse_delta << '\n';
  out << std::fixed << std::setprecision(12);
  emit_metric(out, prefix + ".analytic", analytic_metric);
  emit_metric(out, prefix + ".registered", registered_metric);
  const bool passed =
      std::isfinite(prediction.max_abs_delta) &&
      prediction.max_abs_delta <= max_prediction_delta_tolerance &&
      std::fabs(direction_delta) <= metric_delta_tolerance &&
      std::fabs(rank_delta) <= metric_delta_tolerance &&
      std::fabs(rmse_delta) <= metric_delta_tolerance;
  out << prefix << ".pass=" << bool_text(passed) << '\n';
  return passed;
}

bool tensors_equal(const mdn::PerEdgeAffineReturnHeadState &lhs,
                   const mdn::PerEdgeAffineReturnHeadState &rhs) {
  return lhs.feature_mean.equal(rhs.feature_mean) &&
         lhs.feature_inv_std.equal(rhs.feature_inv_std) &&
         lhs.weights.equal(rhs.weights) && lhs.bias.equal(rhs.bias);
}

bool parameters_frozen(const mdn::PerEdgeAffineReturnHead &head) {
  const auto parameters = head->parameters();
  return std::all_of(parameters.begin(), parameters.end(),
                     [](const torch::Tensor &parameter) {
                       return !parameter.requires_grad();
                     });
}

bool emit_affine_calibration_split(std::ostream &out, const std::string &split,
                                   AnalyticRidgeHead &analytic,
                                   mdn::PerEdgeAffineReturnHead &in_memory,
                                   mdn::PerEdgeAffineReturnHead &reloaded,
                                   const Dataset &dataset,
                                   const std::vector<int64_t> &indices,
                                   const Metric &analytic_metric,
                                   const Options &opt) {
  constexpr double max_prediction_delta_tolerance = 1.0e-6;
  constexpr double metric_delta_tolerance = 1.0e-9;
  if (indices.empty()) {
    throw std::runtime_error("affine calibration split must not be empty");
  }
  analytic->eval();
  in_memory->eval();
  reloaded->eval();
  torch::NoGradGuard no_grad;
  const auto selected = index_tensor(indices, torch::Device(torch::kCPU));
  const auto context = dataset.context.index_select(0, selected);
  const auto readout = dataset.readout_features.index_select(0, selected);
  const auto analytic_prediction = analytic->forward(context);
  const auto in_memory_context_prediction = in_memory->forward(context);
  const auto in_memory_readout_prediction =
      in_memory->forward_readout_features(readout);
  const auto reloaded_context_prediction = reloaded->forward(context);
  const auto reloaded_readout_prediction =
      reloaded->forward_readout_features(readout);

  const auto analytic_to_in_memory = compare_prediction_tensors(
      analytic_prediction, in_memory_context_prediction);
  const auto analytic_to_reloaded = compare_prediction_tensors(
      analytic_prediction, reloaded_context_prediction);
  const auto in_memory_context_to_readout = compare_prediction_tensors(
      in_memory_context_prediction, in_memory_readout_prediction);
  const auto in_memory_to_reloaded = compare_prediction_tensors(
      in_memory_context_prediction, reloaded_context_prediction);
  const auto in_memory_readout_to_reloaded = compare_prediction_tensors(
      in_memory_readout_prediction, reloaded_readout_prediction);
  const auto reloaded_context_to_readout = compare_prediction_tensors(
      reloaded_context_prediction, reloaded_readout_prediction);

  const auto reloaded_metric =
      evaluate_head(reloaded, dataset.context, dataset.target, indices, opt);
  const auto direction_delta = metric_directional_accuracy(reloaded_metric) -
                               metric_directional_accuracy(analytic_metric);
  const auto rank_delta = metric_pairwise_rank_accuracy(reloaded_metric) -
                          metric_pairwise_rank_accuracy(analytic_metric);
  const auto rmse_delta =
      metric_rmse(reloaded_metric) - metric_rmse(analytic_metric);
  const auto prefix = "affine_calibration." + split;
  emit_prediction_tensor_parity(out, prefix + ".analytic_vs_in_memory_context",
                                analytic_prediction,
                                in_memory_context_prediction);
  emit_prediction_tensor_parity(out, prefix + ".analytic_vs_reloaded_context",
                                analytic_prediction,
                                reloaded_context_prediction);
  emit_prediction_tensor_parity(
      out, prefix + ".in_memory_context_vs_exact_readout",
      in_memory_context_prediction, in_memory_readout_prediction);
  emit_prediction_tensor_parity(out, prefix + ".in_memory_vs_reloaded_context",
                                in_memory_context_prediction,
                                reloaded_context_prediction);
  emit_prediction_tensor_parity(out, prefix + ".in_memory_vs_reloaded_readout",
                                in_memory_readout_prediction,
                                reloaded_readout_prediction);
  emit_prediction_tensor_parity(out, prefix + ".reloaded_context_vs_readout",
                                reloaded_context_prediction,
                                reloaded_readout_prediction);
  out << prefix << ".reloaded_context_prediction_digest="
      << canonical_prediction_digest(reloaded_context_prediction) << '\n';
  out << prefix << ".reloaded_readout_prediction_digest="
      << canonical_prediction_digest(reloaded_readout_prediction) << '\n';
  out << std::scientific << std::setprecision(17);
  out << prefix << ".directional_accuracy_delta=" << direction_delta << '\n';
  out << prefix << ".pairwise_rank_accuracy_delta=" << rank_delta << '\n';
  out << prefix << ".rmse_delta=" << rmse_delta << '\n';
  out << std::fixed << std::setprecision(12);
  emit_metric(out, prefix + ".analytic", analytic_metric);
  emit_metric(out, prefix + ".reloaded", reloaded_metric);

  const bool passed =
      analytic_to_in_memory.max_abs_delta <= max_prediction_delta_tolerance &&
      analytic_to_reloaded.max_abs_delta <= max_prediction_delta_tolerance &&
      in_memory_context_to_readout.torch_equal &&
      in_memory_to_reloaded.torch_equal &&
      in_memory_readout_to_reloaded.torch_equal &&
      reloaded_context_to_readout.torch_equal &&
      std::fabs(direction_delta) <= metric_delta_tolerance &&
      std::fabs(rank_delta) <= metric_delta_tolerance &&
      std::fabs(rmse_delta) <= metric_delta_tolerance;
  out << prefix << ".pass=" << bool_text(passed) << '\n';
  return passed;
}

bool export_and_verify_affine_calibration(
    std::ostream &out, const Dataset &dataset,
    const std::vector<int64_t> &selection_train_indices,
    const std::vector<int64_t> &validation_indices,
    const std::vector<int64_t> &development_indices,
    const std::vector<int64_t> &diagnostic_confirmation_indices,
    double selected_alpha, AnalyticRidgeHead &analytic,
    const Metric &development_metric, const Metric &confirmation_metric,
    const Options &opt) {
  if (opt.affine_calibration_output_path.empty() ||
      opt.affine_calibration_metadata_output_path.empty()) {
    return true;
  }

  const auto actual_fit_probe_sha256 = sha256_file(opt.input_path);
  if (actual_fit_probe_sha256 != opt.affine_calibration_fit_probe_sha256) {
    throw std::runtime_error(
        "affine calibration fit probe SHA-256 does not match input bytes");
  }
  const auto anchor_count = dataset.context.size(0);
  const auto H = dataset.feature_dim;
  auto quote = dataset.context.select(1, 0).unsqueeze(1).expand(
      {anchor_count, dataset.edge_count, dataset.channel_count, H});
  auto base = dataset.context.narrow(1, 1, dataset.edge_count);
  const auto context_derived_prefix =
      torch::cat({base, quote, base - quote}, /*dim=*/-1).contiguous();
  const auto exact_cached_prefix = dataset.readout_features
                                       .narrow(/*dim=*/3, /*start=*/0,
                                               /*length=*/3 * H)
                                       .contiguous();
  const auto feature_prefix_parity =
      compare_prediction_tensors(context_derived_prefix, exact_cached_prefix);
  if (!feature_prefix_parity.torch_equal) {
    throw std::runtime_error(
        "cached first-3H feature prefix does not exactly match "
        "base/quote/base-minus-quote context layout");
  }
  const auto context_prefix_digest = canonical_float32_tensor_digest(
      context_derived_prefix, "mdn_affine_context_derived_feature_prefix.v1");
  const auto cached_prefix_digest = canonical_float32_tensor_digest(
      exact_cached_prefix, "mdn_affine_context_derived_feature_prefix.v1");

  const mdn::PerEdgeAffineReturnHeadOptions head_options{
      .feature_dim = dataset.feature_dim,
      .edge_count = dataset.edge_count,
      .readout_feature_dim = dataset.source_feature_count,
      .channel_count = dataset.channel_count,
      .quote_node_index = 0,
  };
  const auto analytic_state = mdn::PerEdgeAffineReturnHeadState{
      .feature_mean = analytic->feature_mean.reshape({3 * dataset.feature_dim})
                          .detach()
                          .clone(),
      .feature_inv_std =
          analytic->feature_inv_std.reshape({3 * dataset.feature_dim})
              .detach()
              .clone(),
      .weights = analytic->weights
                     .reshape({dataset.edge_count, 3 * dataset.feature_dim})
                     .detach()
                     .clone(),
      .bias = analytic->bias.reshape({dataset.edge_count}).detach().clone(),
  };
  auto calibration = mdn::PerEdgeAffineReturnHead(head_options, analytic_state);
  calibration->validate_registered_state();

  mdn::PerEdgeAffineReturnHeadArtifactMetadata metadata;
  metadata.feature_dim = dataset.feature_dim;
  metadata.edge_count = dataset.edge_count;
  metadata.readout_feature_dim = dataset.source_feature_count;
  metadata.channel_count = dataset.channel_count;
  metadata.quote_node_index = 0;
  metadata.selected_alpha = selected_alpha;
  metadata.selection_train_begin =
      dataset
          .anchors[static_cast<std::size_t>(selection_train_indices.front())];
  metadata.selection_train_end =
      dataset
          .anchors[static_cast<std::size_t>(selection_train_indices.back())] +
      1;
  metadata.validation_purge_begin = metadata.selection_train_end;
  metadata.validation_purge_end =
      dataset.anchors[static_cast<std::size_t>(validation_indices.front())];
  metadata.validation_begin = metadata.validation_purge_end;
  metadata.validation_end =
      dataset.anchors[static_cast<std::size_t>(validation_indices.back())] + 1;
  metadata.refit_begin =
      dataset.anchors[static_cast<std::size_t>(development_indices.front())];
  metadata.refit_end =
      dataset.anchors[static_cast<std::size_t>(development_indices.back())] + 1;
  metadata.valid_from_anchor = metadata.refit_end;
  metadata.graph_order_fingerprint = opt.affine_calibration_graph_fingerprint;
  metadata.edge_ids = dataset.edge_ids;
  metadata.node_ids.push_back(dataset.quote_node_id);
  metadata.node_ids.insert(metadata.node_ids.end(),
                           dataset.base_node_ids.begin(),
                           dataset.base_node_ids.end());
  metadata.fit_probe_schema_id = opt.affine_calibration_source_schema;
  metadata.fit_probe_sha256 = opt.affine_calibration_fit_probe_sha256;
  metadata.representation_checkpoint_sha256 =
      opt.affine_calibration_representation_checkpoint_sha256;
  metadata.mdn_checkpoint_sha256 = opt.affine_calibration_mdn_checkpoint_sha256;
  metadata.legacy_capture_authority = opt.affine_calibration_capture_authority;
  metadata.semantic_tensor_digest =
      mdn::semantic_tensor_digest(calibration, metadata);
  const auto expected_metadata_text = mdn::canonical_metadata_text(metadata);

  mdn::save_per_edge_affine_return_head(opt.affine_calibration_output_path,
                                        calibration, metadata);
  auto loaded =
      mdn::load_per_edge_affine_return_head(opt.affine_calibration_output_path);
  const auto loaded_metadata_text =
      mdn::canonical_metadata_text(loaded.metadata);
  const auto source_state_copy_exact =
      tensors_equal(analytic_state, calibration->affine_state());
  const auto state_roundtrip_exact =
      tensors_equal(calibration->affine_state(), loaded.head->affine_state());
  const auto metadata_roundtrip_exact =
      expected_metadata_text == loaded_metadata_text;
  const auto semantic_digest_roundtrip_exact =
      metadata.semantic_tensor_digest ==
          loaded.metadata.semantic_tensor_digest &&
      loaded.metadata.semantic_tensor_digest ==
          mdn::semantic_tensor_digest(loaded.head, loaded.metadata);

  if (!opt.affine_calibration_metadata_output_path.parent_path().empty()) {
    std::filesystem::create_directories(
        opt.affine_calibration_metadata_output_path.parent_path());
  }
  {
    std::ofstream metadata_out(opt.affine_calibration_metadata_output_path,
                               std::ios::binary | std::ios::trunc);
    if (!metadata_out) {
      throw std::runtime_error("failed to open affine metadata output");
    }
    metadata_out.write(
        loaded_metadata_text.data(),
        static_cast<std::streamsize>(loaded_metadata_text.size()));
    if (!metadata_out) {
      throw std::runtime_error("failed while writing affine metadata output");
    }
  }
  std::ifstream metadata_in(opt.affine_calibration_metadata_output_path,
                            std::ios::binary);
  std::ostringstream metadata_roundtrip_stream;
  metadata_roundtrip_stream << metadata_in.rdbuf();
  const bool metadata_file_roundtrip_exact =
      metadata_in.good() || metadata_in.eof()
          ? metadata_roundtrip_stream.str() == loaded_metadata_text
          : false;

  out << "affine_calibration.enabled=true\n";
  out << "affine_calibration.artifact_family=" << metadata.artifact_family
      << '\n';
  out << "affine_calibration.schema_version=" << metadata.schema_version
      << '\n';
  out << "affine_calibration.diagnostic_only="
      << bool_text(metadata.diagnostic_only) << '\n';
  out << "affine_calibration.feature_layout=" << metadata.feature_layout
      << '\n';
  out << "affine_calibration.suffix_policy=" << metadata.suffix_policy << '\n';
  out << "affine_calibration.archive_byte_identity_required=false\n";
  out << "affine_calibration.statistics_scope=" << metadata.statistics_scope
      << '\n';
  out << "affine_calibration.selection_train_anchor_range=["
      << metadata.selection_train_begin << ',' << metadata.selection_train_end
      << ")\n";
  out << "affine_calibration.validation_purge_anchor_range=["
      << metadata.validation_purge_begin << ',' << metadata.validation_purge_end
      << ")\n";
  out << "affine_calibration.validation_anchor_range=["
      << metadata.validation_begin << ',' << metadata.validation_end << ")\n";
  out << "affine_calibration.refit_anchor_range=[" << metadata.refit_begin
      << ',' << metadata.refit_end << ")\n";
  out << "affine_calibration.valid_from_anchor=" << metadata.valid_from_anchor
      << '\n';
  out << "affine_calibration.fit_probe_schema_id="
      << metadata.fit_probe_schema_id << '\n';
  out << "affine_calibration.fit_probe_sha256=" << metadata.fit_probe_sha256
      << '\n';
  out << "affine_calibration.fit_probe_sha256_verified=true\n";
  out << "affine_calibration.representation_checkpoint_sha256="
      << metadata.representation_checkpoint_sha256 << '\n';
  out << "affine_calibration.mdn_checkpoint_sha256="
      << metadata.mdn_checkpoint_sha256 << '\n';
  out << "affine_calibration.graph_order_fingerprint="
      << metadata.graph_order_fingerprint << '\n';
  out << "affine_calibration.legacy_capture_authority="
      << metadata.legacy_capture_authority << '\n';
  out << "affine_calibration.semantic_tensor_digest="
      << loaded.metadata.semantic_tensor_digest << '\n';
  out << "affine_calibration.feature_prefix_layout_torch_equal="
      << bool_text(feature_prefix_parity.torch_equal) << '\n';
  out << std::scientific << std::setprecision(17);
  out << "affine_calibration.feature_prefix_layout_max_abs_delta="
      << feature_prefix_parity.max_abs_delta << '\n';
  out << std::fixed << std::setprecision(12);
  out << "affine_calibration.context_feature_prefix_digest="
      << context_prefix_digest << '\n';
  out << "affine_calibration.cached_feature_prefix_digest="
      << cached_prefix_digest << '\n';
  out << "affine_calibration.source_state_copy_exact="
      << bool_text(source_state_copy_exact) << '\n';
  out << "affine_calibration.state_roundtrip_torch_equal="
      << bool_text(state_roundtrip_exact) << '\n';
  out << "affine_calibration.metadata_roundtrip_exact="
      << bool_text(metadata_roundtrip_exact) << '\n';
  out << "affine_calibration.metadata_file_roundtrip_exact="
      << bool_text(metadata_file_roundtrip_exact) << '\n';
  out << "affine_calibration.semantic_digest_roundtrip_exact="
      << bool_text(semantic_digest_roundtrip_exact) << '\n';
  out << "affine_calibration.in_memory_parameters_frozen="
      << bool_text(parameters_frozen(calibration)) << '\n';
  out << "affine_calibration.reloaded_parameters_frozen="
      << bool_text(parameters_frozen(loaded.head)) << '\n';

  calibration->eval();
  loaded.head->eval();
  torch::NoGradGuard no_grad;
  const auto full_in_memory_context = calibration->forward(dataset.context);
  const auto full_in_memory_readout =
      calibration->forward_readout_features(dataset.readout_features);
  const auto full_reloaded_context = loaded.head->forward(dataset.context);
  const auto full_reloaded_readout =
      loaded.head->forward_readout_features(dataset.readout_features);
  emit_prediction_tensor_parity(
      out, "affine_calibration.full_in_memory_context_vs_exact_readout",
      full_in_memory_context, full_in_memory_readout);
  emit_prediction_tensor_parity(
      out, "affine_calibration.full_in_memory_vs_reloaded_context",
      full_in_memory_context, full_reloaded_context);
  emit_prediction_tensor_parity(
      out, "affine_calibration.full_in_memory_vs_reloaded_readout",
      full_in_memory_readout, full_reloaded_readout);
  emit_prediction_tensor_parity(
      out, "affine_calibration.full_reloaded_context_vs_readout",
      full_reloaded_context, full_reloaded_readout);
  out << "affine_calibration.full_reloaded_context_prediction_digest="
      << canonical_prediction_digest(full_reloaded_context) << '\n';
  out << "affine_calibration.full_reloaded_readout_prediction_digest="
      << canonical_prediction_digest(full_reloaded_readout) << '\n';

  const bool full_exact =
      full_in_memory_context.equal(full_in_memory_readout) &&
      full_in_memory_context.equal(full_reloaded_context) &&
      full_in_memory_readout.equal(full_reloaded_readout) &&
      full_reloaded_context.equal(full_reloaded_readout);
  const bool development_pass = emit_affine_calibration_split(
      out, "development", analytic, calibration, loaded.head, dataset,
      development_indices, development_metric, opt);
  const bool confirmation_pass = emit_affine_calibration_split(
      out, "diagnostic_confirmation", analytic, calibration, loaded.head,
      dataset, diagnostic_confirmation_indices, confirmation_metric, opt);
  const bool passed =
      source_state_copy_exact && state_roundtrip_exact &&
      metadata_roundtrip_exact && metadata_file_roundtrip_exact &&
      semantic_digest_roundtrip_exact && parameters_frozen(calibration) &&
      parameters_frozen(loaded.head) && full_exact && development_pass &&
      confirmation_pass;
  out << "affine_calibration.pass=" << bool_text(passed) << '\n';
  return passed;
}

struct RidgeCandidate {
  std::size_t index{};
  double alpha{};
  bool solved{false};
  bool gate_feasible{false};
  double max_normalized_residual{};
  double coefficient_l2_norm{};
  Metric validation;
};

bool better_ridge_candidate(const RidgeCandidate &candidate,
                            const RidgeCandidate &incumbent) {
  if (candidate.solved != incumbent.solved) {
    return candidate.solved;
  }
  if (!candidate.solved) {
    return candidate.index < incumbent.index;
  }
  if (candidate.gate_feasible != incumbent.gate_feasible) {
    return candidate.gate_feasible;
  }
  const auto candidate_rmse = metric_rmse(candidate.validation);
  const auto incumbent_rmse = metric_rmse(incumbent.validation);
  const auto tolerance = 1.0e-9 * std::max({1.0, std::fabs(candidate_rmse),
                                            std::fabs(incumbent_rmse)});
  if (std::fabs(candidate_rmse - incumbent_rmse) > tolerance) {
    return candidate_rmse < incumbent_rmse;
  }
  if (candidate.alpha != incumbent.alpha) {
    return candidate.alpha > incumbent.alpha;
  }
  return candidate.index < incumbent.index;
}

void emit_analytic_ridge_calibration(
    std::ostream &out, const Dataset &dataset,
    const std::vector<int64_t> &development_indices,
    const std::vector<int64_t> &diagnostic_confirmation_indices,
    const Options &opt) {
  if (development_indices.size() < 2 ||
      diagnostic_confirmation_indices.empty()) {
    throw std::runtime_error("ridge calibration split is too small");
  }
  const auto development_count =
      static_cast<int64_t>(development_indices.size());
  if (development_count <= opt.ridge_validation_gap + 1) {
    throw std::runtime_error("ridge validation gap consumes development split");
  }
  const auto validation_count = std::clamp(
      static_cast<int64_t>(std::floor(static_cast<double>(development_count) *
                                      opt.ridge_validation_fraction)),
      int64_t{1}, development_count - opt.ridge_validation_gap - 1);
  const auto selection_train_count =
      development_count - opt.ridge_validation_gap - validation_count;
  const auto validation_begin =
      selection_train_count + opt.ridge_validation_gap;
  const std::vector<int64_t> selection_train_indices(
      development_indices.begin(),
      development_indices.begin() + selection_train_count);
  const std::vector<int64_t> validation_indices(development_indices.begin() +
                                                    validation_begin,
                                                development_indices.end());

  out << "ridge.enabled=true\n";
  out << "ridge.variant=per_edge_affine\n";
  out << "ridge.numeric_solver=cpu_float64_cholesky\n";
  out << "ridge.regularization_semantics="
         "mean_squared_error_plus_alpha_times_l2_unpenalized_intercept\n";
  out << "ridge.standardization_scope=selection_fit_only_then_development_"
         "fit_only_for_refit\n";
  out << "ridge.selection_metric=validation_direction_and_rank_gate_then_"
         "raw_target_rmse\n";
  out << "ridge.selection_fallback=raw_target_rmse_when_no_candidate_clears_"
         "the_gate\n";
  out << "ridge.source_probe_authority="
      << (opt.evaluation_input_path.empty()
              ? "protected_evaluation_probe_reused_for_diagnosis"
              : "frozen_train_probe_plus_previously_consumed_protected_eval")
      << '\n';
  out << "ridge.benchmark_acceptance_authority=false\n";
  out << "ridge.diagnostic_confirmation_unseen_by_lambda_selection=true\n";
  out << "ridge.diagnostic_confirmation_is_new_benchmark_test=false\n";
  out << "ridge.selection_train_anchor_count=" << selection_train_indices.size()
      << '\n';
  out << "ridge.validation_purge_anchor_count=" << opt.ridge_validation_gap
      << '\n';
  out << "ridge.validation_anchor_count=" << validation_indices.size() << '\n';
  out << "ridge.diagnostic_confirmation_anchor_count="
      << diagnostic_confirmation_indices.size() << '\n';
  out << "ridge.selection_train_anchor_range=["
      << dataset
             .anchors[static_cast<std::size_t>(selection_train_indices.front())]
      << ","
      << (dataset.anchors[static_cast<std::size_t>(
              selection_train_indices.back())] +
          1)
      << ")\n";
  if (opt.ridge_validation_gap > 0) {
    out << "ridge.validation_purge_anchor_range=["
        << dataset.anchors[static_cast<std::size_t>(
               development_indices[static_cast<std::size_t>(
                   selection_train_count)])]
        << ","
        << (dataset.anchors[static_cast<std::size_t>(
                development_indices[static_cast<std::size_t>(validation_begin -
                                                             1)])] +
            1)
        << ")\n";
  }
  out << "ridge.validation_anchor_range=["
      << dataset.anchors[static_cast<std::size_t>(validation_indices.front())]
      << ","
      << (dataset.anchors[static_cast<std::size_t>(validation_indices.back())] +
          1)
      << ")\n";
  out << "ridge.diagnostic_confirmation_anchor_range=["
      << dataset.anchors[static_cast<std::size_t>(
             diagnostic_confirmation_indices.front())]
      << ","
      << (dataset.anchors[static_cast<std::size_t>(
              diagnostic_confirmation_indices.back())] +
          1)
      << ")\n";
  out << "ridge.alpha_candidate_count=" << opt.ridge_alphas.size() << '\n';
  out << "ridge.selection_direction_gate=" << opt.ridge_direction_gate << '\n';
  out << "ridge.selection_rank_gate=" << opt.ridge_rank_gate << '\n';

  std::vector<RidgeCandidate> candidates;
  candidates.reserve(opt.ridge_alphas.size());
  for (std::size_t index = 0; index < opt.ridge_alphas.size(); ++index) {
    const auto alpha = opt.ridge_alphas[index];
    auto fit =
        fit_analytic_per_edge_ridge(dataset, selection_train_indices, alpha);
    const auto selection_train_metric =
        evaluate_head(fit.head, dataset.context, dataset.target,
                      selection_train_indices, opt);
    const auto validation_metric = evaluate_head(
        fit.head, dataset.context, dataset.target, validation_indices, opt);
    const auto validation_direction =
        validation_metric.count > 0
            ? static_cast<double>(validation_metric.sign_correct) /
                  static_cast<double>(validation_metric.count)
            : 0.0;
    const auto validation_rank =
        validation_metric.pair_count > 0
            ? static_cast<double>(validation_metric.pair_correct) /
                  static_cast<double>(validation_metric.pair_count)
            : 0.0;
    const bool gate_feasible =
        validation_direction >= opt.ridge_direction_gate &&
        validation_rank >= opt.ridge_rank_gate;
    const auto prefix = "ridge.candidate_" + std::to_string(index);
    out << prefix << ".alpha=" << alpha << '\n';
    out << prefix << ".solved=true\n";
    out << prefix << ".selection_gate_feasible=" << bool_text(gate_feasible)
        << '\n';
    out << prefix
        << ".max_normalized_solve_residual=" << fit.max_normalized_residual
        << '\n';
    out << prefix << ".coefficient_l2_norm=" << fit.coefficient_l2_norm << '\n';
    emit_metric(out, prefix + ".selection_train", selection_train_metric);
    emit_metric(out, prefix + ".validation", validation_metric);
    candidates.push_back({index, alpha, true, gate_feasible,
                          fit.max_normalized_residual, fit.coefficient_l2_norm,
                          validation_metric});
  }
  const auto selected = std::max_element(
      candidates.begin(), candidates.end(),
      [](const RidgeCandidate &lhs, const RidgeCandidate &rhs) {
        return better_ridge_candidate(rhs, lhs);
      });
  if (selected == candidates.end() || !selected->solved) {
    throw std::runtime_error("ridge calibration did not solve a candidate");
  }

  auto refit = fit_analytic_per_edge_ridge(dataset, development_indices,
                                           selected->alpha);
  const auto development_metric = evaluate_head(
      refit.head, dataset.context, dataset.target, development_indices, opt);
  const auto confirmation_metric =
      evaluate_head(refit.head, dataset.context, dataset.target,
                    diagnostic_confirmation_indices, opt);
  out << "ridge.selected_candidate_index=" << selected->index << '\n';
  out << "ridge.selected_alpha=" << selected->alpha << '\n';
  out << "ridge.selection_gate_pass=" << bool_text(selected->gate_feasible)
      << '\n';
  out << "ridge.selected_validation_rmse=" << metric_rmse(selected->validation)
      << '\n';
  out << "ridge.refit.max_normalized_solve_residual="
      << refit.max_normalized_residual << '\n';
  out << "ridge.refit.coefficient_l2_norm=" << refit.coefficient_l2_norm
      << '\n';
  emit_metric(out, "ridge.selected.validation", selected->validation);
  emit_metric(out, "ridge.refit.development", development_metric);
  emit_metric(out, "ridge.refit.diagnostic_confirmation", confirmation_metric);

  bool affine_parity_pass = true;
  bool affine_calibration_pass = true;
  if (opt.affine_parity_only) {
    auto registered = make_registered_affine_head(
        refit.head, dataset.feature_dim, dataset.edge_count);
    out << "affine_parity.enabled=true\n";
    out << "affine_parity.scope=analytic_refit_vs_registered_per_edge_linear\n";
    out << "affine_parity.statistics_scope=development_fit_only\n";
    out << "affine_parity.coefficient_source=analytic_ridge_refit\n";
    out << "affine_parity.coefficients_copied=true\n";
    out << "affine_parity.compute_device=cpu\n";
    out << "affine_parity.compute_dtype=float64\n";
    out << "affine_parity.output_dtype=float32\n";
    out << "affine_parity.max_abs_delta_tolerance=0.000001\n";
    out << "affine_parity.metric_delta_tolerance=0.000000001\n";
    const bool development_pass = emit_affine_parity_split(
        out, "development", refit.head, registered, dataset.context,
        dataset.target, development_indices, development_metric, opt);
    const bool confirmation_pass = emit_affine_parity_split(
        out, "diagnostic_confirmation", refit.head, registered, dataset.context,
        dataset.target, diagnostic_confirmation_indices, confirmation_metric,
        opt);
    affine_parity_pass = development_pass && confirmation_pass;
    out << "affine_parity.pass=" << bool_text(affine_parity_pass) << '\n';
    affine_calibration_pass = export_and_verify_affine_calibration(
        out, dataset, selection_train_indices, validation_indices,
        development_indices, diagnostic_confirmation_indices, selected->alpha,
        refit.head, development_metric, confirmation_metric, opt);
  }

  constexpr double diagnostic_signal_floor = 0.58;
  const auto validation_direction =
      selected->validation.count > 0
          ? static_cast<double>(selected->validation.sign_correct) /
                static_cast<double>(selected->validation.count)
          : 0.0;
  const auto validation_rank =
      selected->validation.pair_count > 0
          ? static_cast<double>(selected->validation.pair_correct) /
                static_cast<double>(selected->validation.pair_count)
          : 0.0;
  const auto confirmation_direction =
      confirmation_metric.count > 0
          ? static_cast<double>(confirmation_metric.sign_correct) /
                static_cast<double>(confirmation_metric.count)
          : 0.0;
  const auto confirmation_rank =
      confirmation_metric.pair_count > 0
          ? static_cast<double>(confirmation_metric.pair_correct) /
                static_cast<double>(confirmation_metric.pair_count)
          : 0.0;
  out << "ridge.diagnostic_signal_floor=" << diagnostic_signal_floor << '\n';
  out << "ridge.validation_direction_rank_signal_present="
      << bool_text(validation_direction >= diagnostic_signal_floor &&
                   validation_rank >= diagnostic_signal_floor)
      << '\n';
  out << "ridge.confirmation_direction_rank_signal_present="
      << bool_text(confirmation_direction >= diagnostic_signal_floor &&
                   confirmation_rank >= diagnostic_signal_floor)
      << '\n';
  out << "ridge.diagnostic_stable_direction_rank_signal_present="
      << bool_text(validation_direction >= diagnostic_signal_floor &&
                   validation_rank >= diagnostic_signal_floor &&
                   confirmation_direction >= diagnostic_signal_floor &&
                   confirmation_rank >= diagnostic_signal_floor)
      << '\n';
  if (opt.affine_parity_only &&
      (!affine_parity_pass || !affine_calibration_pass)) {
    out.flush();
    throw std::runtime_error(
        "registered or exportable per-edge affine head failed parity checks");
  }
}

void run(const Options &opt) {
  const auto dataset = read_dataset(opt);
  const auto device = !opt.ridge_only && !opt.affine_parity_only &&
                              !opt.force_cpu && torch::cuda::is_available()
                          ? torch::Device(torch::kCUDA)
                          : torch::Device(torch::kCPU);
  const auto context = dataset.context.to(device);
  const auto target = dataset.target.to(device);
  const int64_t anchor_count = context.size(0);
  const int64_t fit_count =
      !opt.evaluation_input_path.empty()
          ? dataset.primary_anchor_count
          : std::clamp(static_cast<int64_t>(
                           std::floor(anchor_count * opt.fit_fraction)),
                       int64_t{1}, anchor_count - 1);
  if (fit_count <= 0 || fit_count >= anchor_count) {
    throw std::runtime_error("primary/evaluation split is empty");
  }
  const auto fit_indices = integer_range(0, fit_count);
  const auto holdout_indices = integer_range(fit_count, anchor_count);
  const int64_t overfit_count =
      std::clamp(opt.overfit_anchors, int64_t{1}, fit_count);
  const auto overfit_indices = integer_range(0, overfit_count);

  std::filesystem::create_directories(opt.output_path.parent_path());
  std::ofstream out(opt.output_path);
  if (!out) {
    throw std::runtime_error("failed to open output probe: " +
                             opt.output_path.string());
  }
  out << std::setprecision(12) << std::fixed;
  out << "schema_id=" << opt.schema_id << '\n';
  out << "input_probe=" << opt.input_path.string() << '\n';
  out << "evaluation_input_probe=" << opt.evaluation_input_path.string()
      << '\n';
  out << "production_head_header="
      << "wikimyei/inference/expected_value/mdn/mixture_density_network_head.h"
      << '\n';
  out << "probe_boundary=post_adapter_direct_edge_features" << '\n';
  out << "runtime_adapter_reapplied=false" << '\n';
  out << "device=" << device.str() << '\n';
  out << "anchor_count=" << anchor_count << '\n';
  out << "primary_anchor_count=" << dataset.primary_anchor_count << '\n';
  out << "anchor_min=" << dataset.anchors.front() << '\n';
  out << "anchor_max=" << dataset.anchors.back() << '\n';
  out << "fit_anchor_count=" << fit_count << '\n';
  out << "holdout_anchor_count=" << anchor_count - fit_count << '\n';
  out << "fit_anchor_max="
      << dataset.anchors[static_cast<std::size_t>(fit_count - 1)] << '\n';
  out << "holdout_anchor_min="
      << dataset.anchors[static_cast<std::size_t>(fit_count)] << '\n';
  out << "edge_count=" << dataset.edge_count << '\n';
  out << "channel_count=" << dataset.channel_count << '\n';
  out << "feature_dim=" << dataset.feature_dim << '\n';
  out << "source_feature_count=" << dataset.source_feature_count << '\n';
  out << "dynamic_feature_count=" << 3 * dataset.feature_dim << '\n';
  out << "ignored_cached_identity_feature_count="
      << dataset.source_feature_count - 3 * dataset.feature_dim << '\n';
  out << "reconstruction_max_abs_error=" << dataset.reconstruction_max_abs_error
      << '\n';
  out << "quote_consistency_max_abs_error="
      << dataset.quote_consistency_max_abs_error << '\n';
  out << "steps=" << opt.steps << '\n';
  out << "standardized_linear_steps=" << opt.standardized_linear_steps << '\n';
  out << "ridge_only=" << bool_text(opt.ridge_only) << '\n';
  out << "affine_parity_only=" << bool_text(opt.affine_parity_only) << '\n';
  out << "mechanical_production_adam_training_skipped="
      << bool_text(opt.ridge_only || opt.affine_parity_only) << '\n';
  out << "ridge_validation_fraction=" << opt.ridge_validation_fraction << '\n';
  out << "ridge_validation_gap=" << opt.ridge_validation_gap << '\n';
  out << "ridge_direction_gate=" << opt.ridge_direction_gate << '\n';
  out << "ridge_rank_gate=" << opt.ridge_rank_gate << '\n';
  out << "batch_size=" << opt.batch_size << '\n';
  out << "learning_rate=" << opt.learning_rate << '\n';
  out << "grad_clip_norm=" << opt.grad_clip_norm << '\n';
  out << "objective_regression_weight=" << opt.regression_weight << '\n';
  out << "objective_direction_weight=" << opt.direction_weight << '\n';
  out << "objective_rank_weight=" << opt.rank_weight << '\n';
  out << "objective_huber_beta=" << opt.huber_beta << '\n';
  out << "objective_logit_scale=" << opt.logit_scale << '\n';
  out << "objective_target_scale=" << opt.target_scale << '\n';

  if (!opt.ridge_only && !opt.affine_parity_only) {
    const std::string overfit_mode = "edge_embedding_per_edge";
    auto overfit_head =
        make_head(dataset, opt, overfit_mode, device, /*seed_offset=*/1000);
    const auto overfit_train =
        train_head(overfit_head, context, target, overfit_indices,
                   opt.overfit_steps, opt, opt.seed + 1000);
    const auto overfit_metric =
        evaluate_head(overfit_head, context, target, overfit_indices, opt);
    out << "mechanical_overfit.identity_mode=" << overfit_mode << '\n';
    out << "mechanical_overfit.anchor_count=" << overfit_count << '\n';
    out << "mechanical_overfit.steps=" << opt.overfit_steps << '\n';
    out << "mechanical_overfit.initial_loss=" << overfit_train.initial_loss
        << '\n';
    out << "mechanical_overfit.final_loss=" << overfit_train.final_loss << '\n';
    emit_metric(out, "mechanical_overfit", overfit_metric);
    const bool overfit_pass =
        overfit_metric.count > 0 &&
        static_cast<double>(overfit_metric.sign_correct) /
                static_cast<double>(overfit_metric.count) >=
            0.98 &&
        overfit_metric.pair_count > 0 &&
        static_cast<double>(overfit_metric.pair_correct) /
                static_cast<double>(overfit_metric.pair_count) >=
            0.98;
    out << "mechanical_overfit.pass=" << bool_text(overfit_pass) << '\n';

    for (std::size_t mode_index = 0; mode_index < opt.identity_modes.size();
         ++mode_index) {
      const auto &mode = opt.identity_modes[mode_index];
      auto head = make_head(dataset, opt, mode, device,
                            static_cast<int64_t>(mode_index));
      const auto train =
          train_head(head, context, target, fit_indices, opt.steps, opt,
                     opt.seed + static_cast<int64_t>(mode_index));
      const auto fit_metric =
          evaluate_head(head, context, target, fit_indices, opt);
      const auto holdout_metric =
          evaluate_head(head, context, target, holdout_indices, opt);
      const auto prefix = "variant." + mode;
      out << prefix << ".initial_loss=" << train.initial_loss << '\n';
      out << prefix << ".final_loss=" << train.final_loss << '\n';
      emit_metric(out, prefix + ".fit", fit_metric);
      emit_metric(out, prefix + ".holdout", holdout_metric);
    }

    auto standardized_linear_head =
        make_standardized_linear_head(dataset, fit_indices, device, opt);
    const auto standardized_linear_train =
        train_head(standardized_linear_head, context, target, fit_indices,
                   opt.standardized_linear_steps, opt, opt.seed + 2000);
    const auto standardized_linear_fit = evaluate_head(
        standardized_linear_head, context, target, fit_indices, opt);
    const auto standardized_linear_holdout = evaluate_head(
        standardized_linear_head, context, target, holdout_indices, opt);
    out << "control.standardized_per_edge_linear.statistics_scope=fit_only\n";
    out << "control.standardized_per_edge_linear.architecture="
           "per_edge_linear_without_hidden_or_sample_layer_norm\n";
    out << "control.standardized_per_edge_linear.initial_loss="
        << standardized_linear_train.initial_loss << '\n';
    out << "control.standardized_per_edge_linear.final_loss="
        << standardized_linear_train.final_loss << '\n';
    emit_metric(out, "control.standardized_per_edge_linear.fit",
                standardized_linear_fit);
    emit_metric(out, "control.standardized_per_edge_linear.holdout",
                standardized_linear_holdout);
  }

  emit_analytic_ridge_calibration(out, dataset, fit_indices, holdout_indices,
                                  opt);
  out.flush();
  if (!out) {
    throw std::runtime_error("failed while writing output probe");
  }
  std::cout << opt.output_path << '\n';
}

} // namespace

int main(int argc, char **argv) {
  try {
    run(parse_options(argc, argv));
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "torch error: " << error.what() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << '\n';
  }
  return 1;
}
