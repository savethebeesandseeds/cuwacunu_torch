#define main mdn_frozen_affine_objective_ladder_embedded_main
#include "mdn_frozen_affine_objective_ladder.cpp"
#undef main

#include <array>

#include <ATen/ops/linalg_svd.h>

#include "wikimyei/inference/expected_value/mdn/per_edge_conditioned_affine_return_head.h"

namespace {

constexpr std::string_view kBridgeSchema =
    "synthetic_mdn_frozen_affine_deployment_bridge_v1";
constexpr std::string_view kExpectedConditioningSchema =
    "synthetic_mdn_frozen_affine_conditioning_parity_v1";
constexpr std::string_view kExpectedConditioningProbeSha256 =
    "1f218f938d965cdb2ee6ef59dc4152a35e4c41ec3189b95dd47dc16252485fd1";
constexpr std::string_view kExpectedReferenceStateDigest =
    "e977bbd91f879ab03b36394369defdb7d5acad27a00503e903abf197aadbfde5";
constexpr std::string_view kExpectedReferencePredictionDigest =
    "13213b1298134a0c9adfa419106e6b3be99f62fa27f02354f14a79f435c33392";
constexpr std::string_view kExpectedRepresentationCheckpointSha256 =
    "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de";
constexpr std::string_view kExpectedMdnCheckpointSha256 =
    "eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e";
constexpr double kBridgeAlpha = 1.0e-10;
constexpr double kBridgeDeploymentTolerance = 2.0e-5;
constexpr double kExpectedReferenceObjective = 2.70530270559889488e-4;
constexpr double kExpectedTrainedArmToAnalyticMaximumAbsDelta =
    1.86264514923095703e-8;
constexpr int64_t kBridgeFitEnd = 554;
constexpr int64_t kBridgeChunkSize = 37;

struct BridgeOptions {
  std::filesystem::path development_input;
  std::filesystem::path conditioning_probe;
  std::filesystem::path representation_checkpoint;
  std::filesystem::path mdn_checkpoint;
  std::filesystem::path artifact_output;
  std::filesystem::path output;
  std::string graph_order_fingerprint;
  std::string schema_id;
};

BridgeOptions parse_bridge_options(int argc, char **argv) {
  BridgeOptions options;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    auto value = [&](const char *name) {
      if (index + 1 >= argc) {
        throw std::runtime_error(std::string("missing value for ") + name);
      }
      return std::string(argv[++index]);
    };
    if (argument == "--development-input") {
      options.development_input = value("--development-input");
    } else if (argument == "--conditioning-probe") {
      options.conditioning_probe = value("--conditioning-probe");
    } else if (argument == "--representation-checkpoint") {
      options.representation_checkpoint =
          value("--representation-checkpoint");
    } else if (argument == "--mdn-checkpoint") {
      options.mdn_checkpoint = value("--mdn-checkpoint");
    } else if (argument == "--graph-order-fingerprint") {
      options.graph_order_fingerprint = value("--graph-order-fingerprint");
    } else if (argument == "--artifact-output") {
      options.artifact_output = value("--artifact-output");
    } else if (argument == "--output") {
      options.output = value("--output");
    } else if (argument == "--schema-id") {
      options.schema_id = value("--schema-id");
    } else {
      throw std::runtime_error("unknown deployment-bridge argument: " +
                               argument);
    }
  }
  if (options.development_input.empty() ||
      options.conditioning_probe.empty() ||
      options.representation_checkpoint.empty() ||
      options.mdn_checkpoint.empty() || options.artifact_output.empty() ||
      options.output.empty() || options.graph_order_fingerprint.empty() ||
      options.schema_id.empty()) {
    throw std::runtime_error(
        "all deployment-bridge inputs, outputs, graph fingerprint, and "
        "schema id are required");
  }
  if (options.graph_order_fingerprint != kExpectedGraphFingerprint) {
    throw std::runtime_error("graph-order fingerprint is not canonical");
  }
  const std::set<std::filesystem::path> paths{
      normalized_path(options.development_input),
      normalized_path(options.conditioning_probe),
      normalized_path(options.representation_checkpoint),
      normalized_path(options.mdn_checkpoint),
      normalized_path(options.artifact_output),
      normalized_path(options.output)};
  if (paths.size() != 6) {
    throw std::runtime_error(
        "deployment-bridge inputs and outputs must be distinct");
  }
  return options;
}

std::map<std::string, std::string>
read_bridge_binding_probe(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("failed to open the conditioning binding probe");
  }
  const std::string bytes((std::istreambuf_iterator<char>(input)),
                          std::istreambuf_iterator<char>());
  if ((!input.good() && !input.eof()) || bytes.empty() ||
      bytes.back() != '\n' || bytes.find('\r') != std::string::npos ||
      bytes.find('\0') != std::string::npos) {
    throw std::runtime_error("conditioning binding probe is not canonical");
  }
  std::map<std::string, std::string> values;
  std::size_t begin = 0;
  while (begin < bytes.size()) {
    const auto end = bytes.find('\n', begin);
    const auto line = bytes.substr(begin, end - begin);
    const auto separator = line.find('=');
    if (line.empty() || separator == std::string::npos || separator == 0 ||
        separator + 1 >= line.size() ||
        !values.emplace(line.substr(0, separator),
                        line.substr(separator + 1))
             .second) {
      throw std::runtime_error(
          "conditioning binding probe has malformed or duplicate keys");
    }
    begin = end + 1;
  }
  return values;
}

const std::string &bridge_required_value(
    const std::map<std::string, std::string> &values,
    const std::string &key) {
  const auto iterator = values.find(key);
  if (iterator == values.end()) {
    throw std::runtime_error("conditioning binding probe is missing key: " +
                             key);
  }
  return iterator->second;
}

struct BridgeBinding {
  std::string probe_sha256;
  std::string development_sha256;
  std::string state_digest;
  std::string prediction_digest;
  double objective{0.0};
  double trained_arm_to_analytic_maximum_abs_delta{0.0};
  bool deployment_pass{false};
};

BridgeBinding validate_bridge_binding(
    const std::filesystem::path &path,
    const std::string &development_sha256) {
  BridgeBinding binding;
  binding.probe_sha256 = sha256_file(path);
  if (binding.probe_sha256 != kExpectedConditioningProbeSha256) {
    throw std::runtime_error("conditioning binding probe SHA-256 mismatch");
  }
  const auto values = read_bridge_binding_probe(path);
  const auto require = [&](const std::string &key, std::string_view expected) {
    if (bridge_required_value(values, key) != expected) {
      throw std::runtime_error("conditioning binding key mismatch: " + key);
    }
  };
  require("schema_id", kExpectedConditioningSchema);
  require("status", "complete");
  require("boundary.development_only", "true");
  require("boundary.maximum_anchor_loaded", "729");
  require("split.fit", "[0,554)");
  require("split.purge", "[554,584)");
  require("split.validation", "[584,730)");
  require("input.development.sha256", development_sha256);
  require("reference.R2_damped_materialized_f32_analytic.state_digest",
          kExpectedReferenceStateDigest);
  require("reference.R2_damped_materialized_f32_analytic.prediction_digest",
          kExpectedReferencePredictionDigest);
  require("reference.R2_damped_materialized_f32_analytic.deployment_parity.pass",
          "false");
  require("arm.R2_damped_lbfgs.analytic_prediction_delta.maximum_abs_delta",
          "1.86264514923095703e-08");
  require("arm.R2_damped_lbfgs.analytic_prediction_delta.reference",
          "reference.R2_damped_materialized_f32_analytic");
  binding.development_sha256 = development_sha256;
  binding.state_digest = bridge_required_value(
      values,
      "reference.R2_damped_materialized_f32_analytic.state_digest");
  binding.prediction_digest = bridge_required_value(
      values,
      "reference.R2_damped_materialized_f32_analytic.prediction_digest");
  binding.objective = parse_f64_strict(
      bridge_required_value(
          values,
          "reference.R2_damped_materialized_f32_analytic.objective.total"),
      "conditioning reference objective");
  binding.trained_arm_to_analytic_maximum_abs_delta = parse_f64_strict(
      bridge_required_value(
          values,
          "arm.R2_damped_lbfgs.analytic_prediction_delta.maximum_abs_delta"),
      "trained arm to analytic maximum absolute delta");
  binding.deployment_pass =
      bridge_required_value(
          values,
          "reference.R2_damped_materialized_f32_analytic.deployment_parity.pass") ==
      "true";
  if (binding.objective != kExpectedReferenceObjective ||
      binding.trained_arm_to_analytic_maximum_abs_delta !=
          kExpectedTrainedArmToAnalyticMaximumAbsDelta ||
      binding.deployment_pass) {
    throw std::runtime_error("conditioning reference contract mismatch");
  }
  return binding;
}

void bridge_append_float64_tensor(std::string &bytes,
                                  const std::string &name,
                                  const torch::Tensor &input) {
  const auto tensor =
      input.detach().to(torch::kCPU, torch::kFloat64).contiguous();
  if (!torch::isfinite(tensor).all().item<bool>()) {
    throw std::runtime_error("cannot digest a non-finite float64 tensor");
  }
  bytes += "tensor=" + name + "\nrank=" + std::to_string(tensor.dim()) +
           "\n";
  for (int64_t dimension = 0; dimension < tensor.dim(); ++dimension) {
    bytes += "shape." + std::to_string(dimension) + "=" +
             std::to_string(tensor.size(dimension)) + "\n";
  }
  const auto *values = tensor.data_ptr<double>();
  for (int64_t index = 0; index < tensor.numel(); ++index) {
    append_u64_be(bytes, std::bit_cast<std::uint64_t>(values[index]));
  }
}

std::string bridge_tensor_digest_f32(const std::string &role,
                                     const torch::Tensor &tensor) {
  std::string bytes =
      "synthetic_mdn_frozen_affine_deployment_bridge_tensor_float32.v1\nrole=" +
      role + "\n";
  append_float32_tensor(bytes, role, tensor);
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

std::string bridge_tensor_digest_f64(const std::string &role,
                                     const torch::Tensor &tensor) {
  std::string bytes =
      "synthetic_mdn_frozen_affine_deployment_bridge_tensor_float64.v1\nrole=" +
      role + "\n";
  bridge_append_float64_tensor(bytes, role, tensor);
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

std::string bridge_conditioning_state_digest(
    const torch::Tensor &mapped_weights,
    const torch::Tensor &uncentered_standardized_bias) {
  std::string bytes =
      "synthetic_mdn_frozen_affine_conditioning_parity_state.v1\n";
  bridge_append_float64_tensor(bytes, "mapped_weights", mapped_weights);
  bridge_append_float64_tensor(bytes, "uncentered_standardized_bias",
                               uncentered_standardized_bias);
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

std::string bridge_conditioning_transform_digest(
    const torch::Tensor &edge_center,
    const torch::Tensor &damped_transform) {
  std::string bytes =
      "synthetic_mdn_frozen_affine_deployment_bridge_conditioning_transform.v1\n";
  bridge_append_float64_tensor(bytes, "edge_center", edge_center);
  bridge_append_float64_tensor(bytes, "damped_transform",
                               damped_transform);
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

struct BridgeGeometry {
  torch::Tensor center;           // [F], CPU float64.
  torch::Tensor basis;            // [F,F], CPU float64.
  torch::Tensor eigenvalues;      // [F], CPU float64.
  torch::Tensor damped_transform; // [F,F], CPU float64.
};

struct BridgeSurface {
  torch::Tensor normalization_mean;    // [F], CPU float32.
  torch::Tensor normalization_inv_std; // [F], CPU float32.
  torch::Tensor canonical_x;           // [A,E,C,F], CPU float64.
  torch::Tensor target;                // [A,E,C], CPU float64.
  std::vector<BridgeGeometry> geometry;
  std::vector<torch::Tensor> damped_coordinates;
};

void bridge_canonicalize_svd_signs(torch::Tensor &u,
                                   torch::Tensor &basis) {
  torch::NoGradGuard no_grad;
  if (u.dim() != 2 || basis.dim() != 2 ||
      u.size(1) != basis.size(1)) {
    throw std::runtime_error("bridge SVD sign domain mismatch");
  }
  for (int64_t column = 0; column < basis.size(1); ++column) {
    const auto vector = basis.select(1, column);
    const auto pivot = vector.abs().argmax().item<int64_t>();
    if (vector.select(0, pivot).item<double>() < 0.0) {
      vector.mul_(-1.0);
      u.select(1, column).mul_(-1.0);
    }
  }
}

BridgeSurface build_bridge_surface(const Dataset &development) {
  torch::NoGradGuard no_grad;
  BridgeSurface surface;
  const auto fit_indices = integer_range(0, kBridgeFitEnd);
  const auto normalization =
      compute_normalization(development.dynamic_features, fit_indices);
  surface.normalization_mean =
      normalization.mean.to(torch::kCPU, torch::kFloat32).contiguous();
  surface.normalization_inv_std =
      normalization.inv_std.to(torch::kCPU, torch::kFloat32).contiguous();
  const auto canonical_f32 =
      ((development.dynamic_features.to(torch::kCPU, torch::kFloat32) -
        surface.normalization_mean.view({1, 1, 1, -1})) *
       surface.normalization_inv_std.view({1, 1, 1, -1}))
          .contiguous();
  surface.canonical_x = canonical_f32.to(torch::kFloat64).contiguous();
  surface.target =
      development.target.to(torch::kCPU, torch::kFloat64).contiguous();
  if (surface.canonical_x.sizes() !=
          torch::IntArrayRef({kDevelopmentEnd, kEdgeCount, kChannelCount,
                              kDynamicFeatureCount}) ||
      surface.target.sizes() !=
          torch::IntArrayRef(
              {kDevelopmentEnd, kEdgeCount, kChannelCount})) {
    throw std::runtime_error("bridge canonical surface shape mismatch");
  }
  const int64_t fit_rows = kBridgeFitEnd * kChannelCount;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto design = surface.canonical_x.select(1, edge)
                            .contiguous()
                            .reshape({kDevelopmentEnd * kChannelCount,
                                      kDynamicFeatureCount});
    const auto center = design.narrow(0, 0, fit_rows).mean(0);
    const auto centered = (design - center).contiguous();
    const auto fit_centered = centered.narrow(0, 0, fit_rows);
    auto svd = at::linalg_svd(fit_centered, false, c10::nullopt);
    auto u = std::get<0>(svd).to(torch::kFloat64).contiguous();
    const auto singular =
        std::get<1>(svd).to(torch::kFloat64).contiguous();
    auto basis = std::get<2>(svd)
                     .transpose(0, 1)
                     .to(torch::kFloat64)
                     .contiguous();
    bridge_canonicalize_svd_signs(u, basis);
    const auto eigenvalues =
        (singular.pow(2) / static_cast<double>(fit_rows)).contiguous();
    const auto damped_transform =
        (basis *
         (eigenvalues + kBridgeAlpha).rsqrt().view({1, -1}))
            .contiguous();
    if (basis.sizes() !=
            torch::IntArrayRef(
                {kDynamicFeatureCount, kDynamicFeatureCount}) ||
        damped_transform.sizes() !=
            torch::IntArrayRef(
                {kDynamicFeatureCount, kDynamicFeatureCount}) ||
        !torch::isfinite(damped_transform).all().item<bool>()) {
      throw std::runtime_error("bridge damped transform is not full-mode");
    }
    surface.geometry.push_back(
        {.center = center.contiguous(),
         .basis = basis,
         .eigenvalues = eigenvalues,
         .damped_transform = damped_transform});
    surface.damped_coordinates.push_back(
        centered.matmul(damped_transform).contiguous());
  }
  return surface;
}

std::vector<torch::Tensor> bridge_materialize_f32_then_promote(
    const std::vector<torch::Tensor> &values) {
  std::vector<torch::Tensor> result;
  result.reserve(values.size());
  for (const auto &value : values) {
    result.push_back(
        value.to(torch::kFloat32).to(torch::kFloat64).contiguous());
  }
  return result;
}

struct BridgeAnalytic {
  std::vector<torch::Tensor> theta; // E x [F], CPU float64.
  std::vector<torch::Tensor> transform; // E x [F,F], CPU float64.
  std::vector<torch::Tensor> coordinates; // E x [A*C,F], CPU float64.
  torch::Tensor mapped_weights; // [E,F], CPU float64.
  torch::Tensor centered_bias; // [E], CPU float64.
  torch::Tensor uncentered_standardized_bias; // [E], CPU float64.
  torch::Tensor prediction; // [A,E,C], CPU float32.
  double data_mse{0.0};
  double ridge_penalty{0.0};
  double objective{0.0};
};

BridgeAnalytic solve_bridge_analytic(const BridgeSurface &surface) {
  torch::NoGradGuard no_grad;
  std::vector<torch::Tensor> transforms;
  transforms.reserve(surface.geometry.size());
  for (const auto &geometry : surface.geometry) {
    transforms.push_back(geometry.damped_transform);
  }
  auto coordinates =
      bridge_materialize_f32_then_promote(surface.damped_coordinates);
  transforms = bridge_materialize_f32_then_promote(transforms);
  const int64_t fit_rows = kBridgeFitEnd * kChannelCount;
  auto mapped_weights =
      torch::zeros({kEdgeCount, kDynamicFeatureCount}, torch::kFloat64);
  auto centered_bias = torch::zeros({kEdgeCount}, torch::kFloat64);
  std::vector<torch::Tensor> parameters;
  std::vector<torch::Tensor> edge_predictions;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto &coordinate = coordinates[static_cast<std::size_t>(edge)];
    const auto &transform = transforms[static_cast<std::size_t>(edge)];
    if (coordinate.sizes() !=
            torch::IntArrayRef({kDevelopmentEnd * kChannelCount,
                                kDynamicFeatureCount}) ||
        transform.sizes() !=
            torch::IntArrayRef(
                {kDynamicFeatureCount, kDynamicFeatureCount})) {
      throw std::runtime_error("bridge analytic full-mode shape mismatch");
    }
    const auto z = coordinate.narrow(0, 0, fit_rows);
    const auto z_mean = z.mean(0);
    const auto z_centered = z - z_mean;
    const auto y = surface.target.narrow(0, 0, kBridgeFitEnd)
                       .select(1, edge)
                       .contiguous()
                       .reshape({fit_rows});
    const auto y_mean = y.mean();
    const auto y_centered = y - y_mean;
    const auto augmented_design = torch::cat(
        {z_centered,
         std::sqrt(static_cast<double>(fit_rows) * kBridgeAlpha) *
             transform},
        0);
    const auto augmented_target = torch::cat(
        {y_centered,
         torch::zeros({kDynamicFeatureCount}, torch::kFloat64)},
        0);
    auto svd = at::linalg_svd(augmented_design, false, c10::nullopt);
    auto u = std::get<0>(svd).to(torch::kFloat64).contiguous();
    const auto singular = std::get<1>(svd);
    auto v = std::get<2>(svd)
                 .transpose(0, 1)
                 .to(torch::kFloat64)
                 .contiguous();
    bridge_canonicalize_svd_signs(u, v);
    const double tolerance =
        std::numeric_limits<double>::epsilon() *
        static_cast<double>(std::max(augmented_design.size(0),
                                     augmented_design.size(1))) *
        singular.select(0, 0).item<double>();
    if ((singular <= tolerance).any().item<bool>()) {
      throw std::runtime_error("bridge analytic ridge system lost rank");
    }
    const auto theta =
        v.matmul((u.transpose(0, 1)
                      .matmul(augmented_target.unsqueeze(1))
                      .squeeze(1)) /
                 singular);
    const auto mapped = transform.matmul(theta);
    const auto intercept = y_mean - z_mean.dot(theta);
    parameters.push_back(theta.contiguous());
    mapped_weights.select(0, edge).copy_(mapped);
    centered_bias.select(0, edge).copy_(intercept);
    edge_predictions.push_back(
        (coordinate.matmul(theta) + intercept)
            .reshape({kDevelopmentEnd, kChannelCount}));
  }
  const auto exact_prediction =
      torch::stack(edge_predictions, 1).to(torch::kFloat64).contiguous();
  const auto prediction = exact_prediction.to(torch::kFloat32).contiguous();
  auto uncentered_standardized_bias = centered_bias.clone();
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    uncentered_standardized_bias.select(0, edge).sub_(
        surface.geometry[static_cast<std::size_t>(edge)].center.dot(
            mapped_weights.select(0, edge)));
  }
  const auto fit_prediction = exact_prediction.narrow(0, 0, kBridgeFitEnd);
  const auto fit_target = surface.target.narrow(0, 0, kBridgeFitEnd);
  const double data_mse =
      (fit_prediction - fit_target).pow(2).mean().item<double>();
  const double ridge_penalty =
      kBridgeAlpha / static_cast<double>(kEdgeCount) *
      mapped_weights.pow(2).sum().item<double>();
  return {.theta = std::move(parameters),
          .transform = std::move(transforms),
          .coordinates = std::move(coordinates),
          .mapped_weights = mapped_weights,
          .centered_bias = centered_bias,
          .uncentered_standardized_bias = uncentered_standardized_bias,
          .prediction = prediction,
          .data_mse = data_mse,
          .ridge_penalty = ridge_penalty,
          .objective = data_mse + ridge_penalty};
}

torch::Tensor bridge_normalized_prefix(
    const torch::Tensor &readout_features, const BridgeSurface &surface,
    const torch::Device &device) {
  if (readout_features.dim() != 4 ||
      readout_features.size(1) != kEdgeCount ||
      readout_features.size(2) != kChannelCount ||
      readout_features.size(3) < kDynamicFeatureCount ||
      readout_features.scalar_type() != torch::kFloat32) {
    throw std::runtime_error("bridge live readout tensor shape mismatch");
  }
  const auto live = readout_features.to(device, torch::kFloat32);
  const auto prefix = live.narrow(3, 0, kDynamicFeatureCount);
  return ((prefix -
           surface.normalization_mean
               .to(device, torch::kFloat32)
               .view({1, 1, 1, kDynamicFeatureCount})) *
          surface.normalization_inv_std
              .to(device, torch::kFloat32)
              .view({1, 1, 1, kDynamicFeatureCount}))
      .contiguous();
}

torch::Tensor bridge_unsafe_uncentered_raw_f32(
    const torch::Tensor &readout_features, const BridgeSurface &surface,
    const BridgeAnalytic &analytic, const torch::Device &device) {
  torch::NoGradGuard no_grad;
  const auto normalized =
      bridge_normalized_prefix(readout_features, surface, device);
  const auto weights =
      analytic.mapped_weights.to(device, torch::kFloat32);
  const auto bias = analytic.uncentered_standardized_bias.to(
      device, torch::kFloat32);
  return ((normalized *
           weights.view({1, kEdgeCount, 1, kDynamicFeatureCount}))
              .sum(3) +
          bias.view({1, kEdgeCount, 1}))
      .contiguous();
}

torch::Tensor bridge_live_centered_mapped_f32(
    const torch::Tensor &readout_features, const BridgeSurface &surface,
    const BridgeAnalytic &analytic, const torch::Device &device) {
  torch::NoGradGuard no_grad;
  const auto normalized =
      bridge_normalized_prefix(readout_features, surface, device);
  std::vector<torch::Tensor> centers;
  for (const auto &geometry : surface.geometry) {
    centers.push_back(geometry.center);
  }
  const auto center =
      torch::stack(centers, 0).to(device, torch::kFloat32);
  const auto weights =
      analytic.mapped_weights.to(device, torch::kFloat32);
  const auto bias = analytic.centered_bias.to(device, torch::kFloat32);
  return (((normalized -
            center.view({1, kEdgeCount, 1, kDynamicFeatureCount})) *
           weights.view({1, kEdgeCount, 1, kDynamicFeatureCount}))
              .sum(3) +
          bias.view({1, kEdgeCount, 1}))
      .contiguous();
}

torch::Tensor bridge_live_explicit_damped_f32(
    const torch::Tensor &readout_features, const BridgeSurface &surface,
    const BridgeAnalytic &analytic, const torch::Device &device) {
  torch::NoGradGuard no_grad;
  const auto normalized =
      bridge_normalized_prefix(readout_features, surface, device);
  const int64_t anchors = normalized.size(0);
  std::vector<torch::Tensor> edges;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto centered =
        normalized.select(1, edge)
            .reshape({anchors * kChannelCount, kDynamicFeatureCount}) -
        surface.geometry[static_cast<std::size_t>(edge)]
            .center.to(device, torch::kFloat32);
    const auto transformed =
        centered.matmul(analytic.transform[static_cast<std::size_t>(edge)]
                            .to(device, torch::kFloat32));
    const auto prediction =
        transformed.matmul(analytic.theta[static_cast<std::size_t>(edge)]
                               .to(device, torch::kFloat32)) +
        analytic.centered_bias.select(0, edge).to(device, torch::kFloat32);
    edges.push_back(prediction.reshape({anchors, kChannelCount}));
  }
  return torch::stack(edges, 1).contiguous();
}

torch::Tensor bridge_live_centered_mapped_f64(
    const torch::Tensor &readout_features, const BridgeSurface &surface,
    const BridgeAnalytic &analytic, const torch::Device &device) {
  torch::NoGradGuard no_grad;
  const auto normalized_f32 =
      bridge_normalized_prefix(readout_features, surface, device);
  std::vector<torch::Tensor> centers;
  for (const auto &geometry : surface.geometry) {
    centers.push_back(geometry.center);
  }
  const auto center = torch::stack(centers, 0).to(device, torch::kFloat64);
  const auto centered_f64 =
      normalized_f32.to(torch::kFloat64) -
      center.view({1, kEdgeCount, 1, kDynamicFeatureCount});
  const auto output_f64 =
      (centered_f64 *
       analytic.mapped_weights
           .to(device, torch::kFloat64)
           .view({1, kEdgeCount, 1, kDynamicFeatureCount}))
          .sum(3) +
      analytic.centered_bias
          .to(device, torch::kFloat64)
          .view({1, kEdgeCount, 1});
  return output_f64.to(torch::kFloat32).contiguous();
}

torch::Tensor bridge_materialized_direct_f32(
    const BridgeAnalytic &analytic, const torch::Device &device) {
  torch::NoGradGuard no_grad;
  std::vector<torch::Tensor> edges;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto prediction =
        analytic.coordinates[static_cast<std::size_t>(edge)]
                .to(device, torch::kFloat32)
                .matmul(analytic.theta[static_cast<std::size_t>(edge)]
                            .to(device, torch::kFloat32)) +
        analytic.centered_bias.select(0, edge).to(device, torch::kFloat32);
    edges.push_back(
        prediction.reshape({kDevelopmentEnd, kChannelCount}));
  }
  return torch::stack(edges, 1).contiguous();
}

template <typename Forward>
torch::Tensor bridge_chunked(const torch::Tensor &readout_features,
                             Forward &&forward) {
  std::vector<torch::Tensor> chunks;
  for (int64_t begin = 0; begin < readout_features.size(0);
       begin += kBridgeChunkSize) {
    const int64_t count =
        std::min(kBridgeChunkSize, readout_features.size(0) - begin);
    chunks.push_back(forward(readout_features.narrow(0, begin, count)));
  }
  return torch::cat(chunks, 0).contiguous();
}

struct BridgeDelta {
  bool torch_equal{false};
  double maximum_abs_delta{0.0};
  double mean_abs_delta{0.0};
  double rmse_delta{0.0};
  bool pass{false};
};

BridgeDelta bridge_delta(const torch::Tensor &lhs_input,
                         const torch::Tensor &rhs_input) {
  const auto lhs =
      lhs_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  const auto rhs =
      rhs_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  if (lhs.sizes() != rhs.sizes() || !torch::isfinite(lhs).all().item<bool>() ||
      !torch::isfinite(rhs).all().item<bool>()) {
    throw std::runtime_error("bridge parity tensor domain mismatch");
  }
  const auto delta = lhs - rhs;
  const double maximum = delta.abs().max().item<double>();
  return {.torch_equal = torch::equal(lhs, rhs),
          .maximum_abs_delta = maximum,
          .mean_abs_delta = delta.abs().mean().item<double>(),
          .rmse_delta = delta.pow(2).mean().sqrt().item<double>(),
          .pass = maximum <= kBridgeDeploymentTolerance};
}

void emit_bridge_delta(std::ostream &output, const std::string &prefix,
                       const std::string &lhs, const std::string &rhs,
                       const std::string &scope,
                       const BridgeDelta &delta) {
  output << prefix << ".lhs=" << lhs << '\n';
  output << prefix << ".rhs=" << rhs << '\n';
  output << prefix << ".scope=" << scope << '\n';
  output << prefix << ".torch_equal=" << bool_text(delta.torch_equal)
         << '\n';
  output << prefix << ".maximum_abs_delta=" << delta.maximum_abs_delta
         << '\n';
  output << prefix << ".mean_abs_delta=" << delta.mean_abs_delta << '\n';
  output << prefix << ".rmse_delta=" << delta.rmse_delta << '\n';
  output << prefix << ".threshold=" << kBridgeDeploymentTolerance << '\n';
  output << prefix << ".pass=" << bool_text(delta.pass) << '\n';
}

void emit_bridge_parity(std::ostream &output, const std::string &prefix,
                        const std::string &lhs_name,
                        const std::string &rhs_name,
                        const torch::Tensor &lhs,
                        const torch::Tensor &rhs) {
  emit_bridge_delta(output, prefix, lhs_name, rhs_name, "[0,730)",
                    bridge_delta(lhs, rhs));
  emit_bridge_delta(
      output, prefix + ".validation", lhs_name, rhs_name, "[584,730)",
      bridge_delta(lhs.narrow(0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
                   rhs.narrow(0, kPurgeEnd,
                              kDevelopmentEnd - kPurgeEnd)));
}

void emit_bridge_arm(std::ostream &output, const std::string &prefix,
                     const std::string &operation_order,
                     const torch::Tensor &prediction,
                     const torch::Tensor &canonical_reference,
                     const torch::Tensor &centered_reference) {
  output << prefix << ".operation_order=" << operation_order << '\n';
  output << prefix << ".prediction_digest="
         << prediction_digest(prediction) << '\n';
  emit_bridge_parity(output, prefix + ".to_canonical_reference", prefix,
                     "reference.canonical_materialized_damped_f64",
                     prediction, canonical_reference);
  emit_bridge_parity(output, prefix + ".to_centered_f64_reference", prefix,
                     "reference.live_centered_mapped_f64", prediction,
                     centered_reference);
}

mdn::PerEdgeConditionedAffineReturnHeadArtifactMetadata
make_bridge_metadata(const BridgeOptions &options,
                     const Dataset &development,
                     const BridgeBinding &binding,
                     const std::string &representation_sha256,
                     const std::string &mdn_sha256,
                     const std::string &development_sha256,
                     const std::string &transform_digest) {
  mdn::PerEdgeConditionedAffineReturnHeadArtifactMetadata metadata;
  metadata.feature_dim = kDynamicFeatureCount;
  metadata.edge_count = kEdgeCount;
  metadata.channel_count = kChannelCount;
  metadata.conditioning_alpha = kBridgeAlpha;
  metadata.fit_begin = 0;
  metadata.fit_end = kBridgeFitEnd;
  metadata.purge_begin = kBridgeFitEnd;
  metadata.purge_end = kPurgeEnd;
  metadata.validation_begin = kPurgeEnd;
  metadata.validation_end = kDevelopmentEnd;
  metadata.max_anchor = kDevelopmentEnd - 1;
  metadata.graph_order_fingerprint = options.graph_order_fingerprint;
  metadata.edge_ids = development.edge_ids;
  metadata.node_ids.push_back(development.quote_node_id);
  metadata.node_ids.insert(metadata.node_ids.end(),
                           development.base_node_ids.begin(),
                           development.base_node_ids.end());
  metadata.source_probe_schema_id = std::string(kFitProbeSchema);
  metadata.source_probe_sha256 = development_sha256;
  metadata.conditioning_probe_schema_id =
      std::string(kExpectedConditioningSchema);
  metadata.conditioning_probe_sha256 = binding.probe_sha256;
  metadata.representation_checkpoint_sha256 = representation_sha256;
  metadata.mdn_checkpoint_sha256 = mdn_sha256;
  metadata.conditioning_transform_schema_id =
      "synthetic_mdn_frozen_affine_deployment_bridge_conditioning_transform.v1";
  metadata.conditioning_transform_sha256 = transform_digest;
  return metadata;
}

struct BridgeArtifactStaging {
  std::filesystem::path temporary;
  std::filesystem::path final;
  bool committed{false};

  ~BridgeArtifactStaging() {
    if (!committed) {
      std::error_code error;
      if (!temporary.empty()) {
        std::filesystem::remove(temporary, error);
      }
      error.clear();
      if (!final.empty()) {
        std::filesystem::remove(final, error);
      }
    }
  }
};

} // namespace

int main(int argc, char **argv) {
  try {
    std::locale::global(std::locale::classic());
    const auto options = parse_bridge_options(argc, argv);
    if (options.schema_id != kBridgeSchema) {
      throw std::runtime_error(
          "deployment-bridge schema does not match the contract");
    }
    const auto output_temporary =
        std::filesystem::path(options.output.string() + ".tmp");
    const auto artifact_temporary =
        std::filesystem::path(options.artifact_output.string() + ".tmp");
    if (std::filesystem::exists(options.output) ||
        std::filesystem::exists(output_temporary) ||
        std::filesystem::exists(options.artifact_output) ||
        std::filesystem::exists(artifact_temporary)) {
      throw std::runtime_error(
          "refusing to replace a deployment-bridge output");
    }
    BridgeArtifactStaging artifact_staging{artifact_temporary,
                                            options.artifact_output};

    const auto runtime = configure_deterministic_runtime();
    const torch::Device cpu(torch::kCPU);
    const torch::Device cuda(torch::kCUDA, 0);
    const auto development_sha256 = sha256_file(options.development_input);
    const auto representation_sha256 =
        sha256_file(options.representation_checkpoint);
    const auto mdn_sha256 = sha256_file(options.mdn_checkpoint);
    if (representation_sha256 != kExpectedRepresentationCheckpointSha256 ||
        mdn_sha256 != kExpectedMdnCheckpointSha256) {
      throw std::runtime_error("checkpoint provenance SHA-256 mismatch");
    }
    const auto binding = validate_bridge_binding(
        options.conditioning_probe, development_sha256);
    const auto development =
        read_probe(options.development_input, 0, kDevelopmentEnd);
    if (development.record_schema != kProbeRecordSchema ||
        development.anchors.back() != kDevelopmentEnd - 1) {
      throw std::runtime_error("development probe schema is not frozen");
    }

    const auto surface = build_bridge_surface(development);
    const auto analytic = solve_bridge_analytic(surface);
    const auto state_digest = bridge_conditioning_state_digest(
        analytic.mapped_weights, analytic.uncentered_standardized_bias);
    const auto canonical_prediction_digest =
        prediction_digest(analytic.prediction);
    if (state_digest != binding.state_digest ||
        state_digest != kExpectedReferenceStateDigest ||
        canonical_prediction_digest != binding.prediction_digest ||
        canonical_prediction_digest != kExpectedReferencePredictionDigest ||
        analytic.objective != binding.objective ||
        analytic.objective != kExpectedReferenceObjective) {
      throw std::runtime_error(
          "reconstructed conditioning reference identity mismatch");
    }
    for (const auto &geometry : surface.geometry) {
      if (geometry.damped_transform.size(0) != kDynamicFeatureCount ||
          geometry.damped_transform.size(1) != kDynamicFeatureCount) {
        throw std::runtime_error("conditioning transform lost a mode");
      }
    }

    const auto raw_cpu = development.readout_features.contiguous();
    const auto raw_cuda = raw_cpu.to(cuda, torch::kFloat32);
    const auto canonical_reference = analytic.prediction;
    const auto direct_materialized_cpu =
        bridge_materialized_direct_f32(analytic, cpu).to(torch::kCPU);
    const auto direct_materialized_cuda =
        bridge_materialized_direct_f32(analytic, cuda).to(torch::kCPU);
    const auto unsafe_raw_cuda = bridge_unsafe_uncentered_raw_f32(
                                     raw_cuda, surface, analytic, cuda)
                                     .to(torch::kCPU);
    const auto centered_f32_cuda = bridge_live_centered_mapped_f32(
                                       raw_cuda, surface, analytic, cuda)
                                       .to(torch::kCPU);
    const auto explicit_damped_cuda = bridge_live_explicit_damped_f32(
                                          raw_cuda, surface, analytic, cuda)
                                          .to(torch::kCPU);
    const auto centered_f64_cpu = bridge_live_centered_mapped_f64(
        raw_cpu, surface, analytic, cpu);
    const auto centered_f64_cuda = bridge_live_centered_mapped_f64(
                                       raw_cuda, surface, analytic, cuda)
                                       .to(torch::kCPU);
    const auto centered_f64_cuda_chunked =
        bridge_chunked(raw_cuda, [&](const torch::Tensor &chunk) {
          return bridge_live_centered_mapped_f64(chunk, surface, analytic,
                                                  cuda);
        })
            .to(torch::kCPU);

    std::vector<torch::Tensor> centers;
    std::vector<torch::Tensor> transforms;
    std::vector<torch::Tensor> thetas;
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      centers.push_back(
          surface.geometry[static_cast<std::size_t>(edge)].center);
      transforms.push_back(analytic.transform[static_cast<std::size_t>(edge)]);
      thetas.push_back(analytic.theta[static_cast<std::size_t>(edge)]);
    }
    const auto edge_center = torch::stack(centers, 0).contiguous();
    const auto damped_transform = torch::stack(transforms, 0).contiguous();
    const auto theta = torch::stack(thetas, 0).contiguous();
    const auto transform_digest = bridge_conditioning_transform_digest(
        edge_center, damped_transform);
    const mdn::PerEdgeConditionedAffineReturnHeadOptions head_options{
        .feature_dim = kDynamicFeatureCount,
        .edge_count = kEdgeCount,
        .channel_count = kChannelCount,
    };
    const mdn::PerEdgeConditionedAffineReturnHeadState head_state{
        .feature_mean = surface.normalization_mean.clone(),
        .feature_inv_std = surface.normalization_inv_std.clone(),
        .edge_center = edge_center.clone(),
        .mapped_weights = analytic.mapped_weights.clone(),
        .centered_bias = analytic.centered_bias.clone(),
    };
    auto metadata = make_bridge_metadata(
        options, development, binding, representation_sha256, mdn_sha256,
        development_sha256, transform_digest);
    auto in_memory =
        mdn::PerEdgeConditionedAffineReturnHead(head_options, head_state);
    in_memory->eval();
    in_memory->validate_registered_state();
    const auto artifact_source_semantic_digest =
        mdn::conditioned_affine_semantic_tensor_digest(in_memory, metadata);
    const auto in_memory_cpu_full =
        in_memory->forward_readout_features(raw_cpu).to(torch::kCPU);
    const auto in_memory_cpu_chunked =
        bridge_chunked(raw_cpu, [&](const torch::Tensor &chunk) {
          return in_memory->forward_readout_features(chunk);
        }).to(torch::kCPU);

    mdn::save_per_edge_conditioned_affine_return_head(
        artifact_temporary, in_memory, metadata);
    auto reloaded_cpu = mdn::load_per_edge_conditioned_affine_return_head(
        artifact_temporary);
    if (reloaded_cpu.metadata.semantic_tensor_digest !=
            artifact_source_semantic_digest ||
        reloaded_cpu.metadata.conditioning_transform_schema_id !=
            metadata.conditioning_transform_schema_id ||
        reloaded_cpu.metadata.conditioning_transform_sha256 !=
            transform_digest ||
        mdn::conditioned_affine_semantic_tensor_digest(
            reloaded_cpu.head, reloaded_cpu.metadata) !=
            artifact_source_semantic_digest) {
      throw std::runtime_error("CPU archive reload semantic digest mismatch");
    }
    const auto archive_cpu_full =
        reloaded_cpu.head->forward_readout_features(raw_cpu).to(torch::kCPU);
    const auto archive_cpu_chunked =
        bridge_chunked(raw_cpu, [&](const torch::Tensor &chunk) {
          return reloaded_cpu.head->forward_readout_features(chunk);
        }).to(torch::kCPU);

    in_memory->to(cuda);
    in_memory->eval();
    in_memory->validate_registered_state();
    const auto in_memory_cuda_full =
        in_memory->forward_readout_features(raw_cuda).to(torch::kCPU);
    const auto in_memory_cuda_chunked =
        bridge_chunked(raw_cuda, [&](const torch::Tensor &chunk) {
          return in_memory->forward_readout_features(chunk);
        }).to(torch::kCPU);
    auto reloaded_cuda = mdn::load_per_edge_conditioned_affine_return_head(
        artifact_temporary);
    reloaded_cuda.head->to(cuda);
    reloaded_cuda.head->eval();
    reloaded_cuda.head->validate_registered_state();
    if (reloaded_cuda.metadata.semantic_tensor_digest !=
            artifact_source_semantic_digest ||
        reloaded_cuda.metadata.conditioning_transform_schema_id !=
            metadata.conditioning_transform_schema_id ||
        reloaded_cuda.metadata.conditioning_transform_sha256 !=
            transform_digest ||
        mdn::conditioned_affine_semantic_tensor_digest(
            reloaded_cuda.head, reloaded_cuda.metadata) !=
            artifact_source_semantic_digest) {
      throw std::runtime_error("CUDA archive reload semantic digest mismatch");
    }
    const auto archive_cuda_full =
        reloaded_cuda.head->forward_readout_features(raw_cuda).to(torch::kCPU);
    const auto archive_cuda_chunked =
        bridge_chunked(raw_cuda, [&](const torch::Tensor &chunk) {
          return reloaded_cuda.head->forward_readout_features(chunk);
        }).to(torch::kCPU);

    auto suffix_changed_cpu = raw_cpu.clone();
    auto ignored_suffix = suffix_changed_cpu.narrow(
        3, kDynamicFeatureCount,
        kSourceFeatureCount - kDynamicFeatureCount);
    for (int64_t index = 0; index < ignored_suffix.size(3); ++index) {
      ignored_suffix.select(3, index).fill_(
          index % 2 == 0 ? std::numeric_limits<float>::quiet_NaN()
                         : std::numeric_limits<float>::infinity());
    }
    const auto suffix_changed_cuda = suffix_changed_cpu.to(cuda);
    const auto archive_cpu_suffix =
        reloaded_cpu.head->forward_readout_features(suffix_changed_cpu)
            .to(torch::kCPU);
    const auto archive_cuda_suffix =
        reloaded_cuda.head->forward_readout_features(suffix_changed_cuda)
            .to(torch::kCPU);

    const auto parity_cpu_reference_to_in_memory =
        bridge_delta(canonical_reference, in_memory_cpu_full);
    const auto parity_cpu_to_cuda =
        bridge_delta(in_memory_cpu_full, in_memory_cuda_full);
    const auto parity_cpu_full_to_chunked =
        bridge_delta(in_memory_cpu_full, in_memory_cpu_chunked);
    const auto parity_cuda_full_to_chunked =
        bridge_delta(in_memory_cuda_full, in_memory_cuda_chunked);
    const auto parity_in_memory_to_archive_reload =
        bridge_delta(in_memory_cuda_full, archive_cuda_full);
    const auto parity_cpu_archive =
        bridge_delta(in_memory_cpu_full, archive_cpu_full);
    const auto parity_archive_cpu_chunk =
        bridge_delta(archive_cpu_full, archive_cpu_chunked);
    const auto parity_archive_cuda_chunk =
        bridge_delta(archive_cuda_full, archive_cuda_chunked);
    const auto parity_live_centered_cpu =
        bridge_delta(centered_f64_cpu, in_memory_cpu_full);
    const auto parity_live_centered_cuda =
        bridge_delta(centered_f64_cuda, in_memory_cuda_full);
    const auto parity_canonical_to_centered_cuda =
        bridge_delta(canonical_reference, centered_f64_cuda);
    const auto parity_suffix_cpu =
        bridge_delta(archive_cpu_full, archive_cpu_suffix);
    const auto parity_suffix_cuda =
        bridge_delta(archive_cuda_full, archive_cuda_suffix);
    const auto unsafe_to_canonical =
        bridge_delta(unsafe_raw_cuda, canonical_reference);
    if (unsafe_to_canonical.pass) {
      throw std::runtime_error(
          "unsafe uncentered float32 collapse unexpectedly passed");
    }

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
    probe << "runtime.cuda_device_name=" << runtime.cuda_device_name << '\n';
    probe << "runtime.cpu_thread_count=1\n";
    probe << "runtime.cpu_interop_thread_count=1\n";
    probe << "runtime.deterministic_algorithms="
          << bool_text(at::globalContext().deterministicAlgorithms()) << '\n';
    probe << "runtime.allow_tf32_cublas="
          << bool_text(at::globalContext().allowTF32CuBLAS()) << '\n';
    probe << "runtime.allow_tf32_cudnn="
          << bool_text(at::globalContext().allowTF32CuDNN()) << '\n';
    probe << "runtime.seed=" << kSeed << '\n';

    probe << "input.development.sha256=" << development_sha256 << '\n';
    probe << "input.conditioning_probe.sha256=" << binding.probe_sha256
          << '\n';
    probe << "input.representation_checkpoint.sha256="
          << representation_sha256 << '\n';
    probe << "input.mdn_checkpoint.sha256=" << mdn_sha256 << '\n';
    probe << "input.graph_order_fingerprint="
          << options.graph_order_fingerprint << '\n';
    probe << "data.record_schema=" << development.record_schema << '\n';
    probe << "data.readout_shape=[730,3,3,400]\n";
    probe << "data.live_readout_digest="
          << bridge_tensor_digest_f32("live_readout_features", raw_cpu)
          << '\n';
    probe << "data.dynamic_prefix_digest="
          << bridge_tensor_digest_f32(
                 "dynamic_prefix",
                 raw_cpu.narrow(3, 0, kDynamicFeatureCount))
          << '\n';
    probe << "data.maximum_anchor_loaded=729\n";
    probe << "boundary.development_only=true\n";
    probe << "boundary.maximum_anchor_loaded=729\n";
    probe << "boundary.holdout.opened=false\n";
    probe << "split.fit=[0,554)\n";
    probe << "split.purge=[554,584)\n";
    probe << "split.validation=[584,730)\n";
    probe << "split.holdout.opened=false\n";

    probe << "execution.alpha=" << kBridgeAlpha << '\n';
    probe << "execution.deployment_threshold="
          << kBridgeDeploymentTolerance << '\n';
    probe << "execution.chunk_size=" << kBridgeChunkSize << '\n';
    probe << "execution.operation_order=readout_float32_prefix_384_then_subtract_mean_float32_then_multiply_inv_std_float32_then_promote_float64_same_device_then_subtract_edge_center_float64_then_dot_mapped_weights_sum_float64_then_add_centered_bias_float64_then_cast_output_float32\n";
    probe << "execution.dtype_roles=input_float32_normalization_float32_coefficients_float64_accumulation_float64_output_float32\n";
    probe << "execution.suffix_policy=ignore_indices_384_through_399\n";
    probe << "execution.suffix_perturbation=alternating_quiet_nan_and_positive_infinity_indices_384_through_399\n";
    probe << "execution.fit_scope=[0,554)\n";
    probe << "execution.validation_scope=[584,730)\n";
    probe << "execution.fit_computation_uses_validation=false\n";

    probe << "source.conditioning_reference.state_digest=" << state_digest
          << '\n';
    probe << "source.conditioning_reference.prediction_digest="
          << canonical_prediction_digest << '\n';
    probe << "source.conditioning_reference.objective.data_mse="
          << analytic.data_mse << '\n';
    probe << "source.conditioning_reference.objective.ridge_penalty="
          << analytic.ridge_penalty << '\n';
    probe << "source.conditioning_reference.objective.total="
          << analytic.objective << '\n';
    probe << "source.conditioning_reference.trained_arm_to_analytic.maximum_abs_delta="
          << binding.trained_arm_to_analytic_maximum_abs_delta << '\n';
    probe << "source.conditioning_reference.trained_arm_to_analytic.threshold="
          << kBridgeDeploymentTolerance << '\n';
    probe << "source.conditioning_reference.trained_arm_to_analytic.pass="
          << bool_text(binding.trained_arm_to_analytic_maximum_abs_delta <=
                       kBridgeDeploymentTolerance)
          << '\n';
    probe << "source.conditioning_reference.reconstructed_state_match=true\n";
    probe << "source.conditioning_reference.reconstructed_prediction_match=true\n";
    probe << "source.conditioning_reference.reconstructed_objective_match=true\n";
    probe << "source.conditioning_transform.schema_id=synthetic_mdn_frozen_affine_deployment_bridge_conditioning_transform.v1\n";
    probe << "source.conditioning_transform.sha256=" << transform_digest
          << '\n';
    probe << "state.normalization_mean.digest="
          << bridge_tensor_digest_f32("normalization_mean",
                                      surface.normalization_mean)
          << '\n';
    probe << "state.normalization_inv_std.digest="
          << bridge_tensor_digest_f32("normalization_inv_std",
                                      surface.normalization_inv_std)
          << '\n';
    probe << "state.edge_center.digest="
          << bridge_tensor_digest_f64("edge_center", edge_center) << '\n';
    probe << "state.damped_transform.digest="
          << bridge_tensor_digest_f64("damped_transform", damped_transform)
          << '\n';
    probe << "state.theta.digest="
          << bridge_tensor_digest_f64("theta", theta) << '\n';
    probe << "state.mapped_weights.digest="
          << bridge_tensor_digest_f64("mapped_weights",
                                      analytic.mapped_weights)
          << '\n';
    probe << "state.centered_bias.digest="
          << bridge_tensor_digest_f64("centered_bias",
                                      analytic.centered_bias)
          << '\n';
    probe << "state.uncentered_standardized_bias.digest="
          << bridge_tensor_digest_f64(
                 "uncentered_standardized_bias",
                 analytic.uncentered_standardized_bias)
          << '\n';
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      probe << "conditioner.edge." << edge << ".rank="
            << kDynamicFeatureCount << '\n';
      probe << "conditioner.edge." << edge << ".mode_count="
            << analytic.transform[static_cast<std::size_t>(edge)].size(1)
            << '\n';
      probe << "conditioner.edge." << edge << ".transform_digest="
            << bridge_tensor_digest_f64(
                   "damped_transform_edge_" + std::to_string(edge),
                   analytic.transform[static_cast<std::size_t>(edge)])
            << '\n';
      probe << "conditioner.edge." << edge << ".theta_digest="
            << bridge_tensor_digest_f64(
                   "theta_edge_" + std::to_string(edge),
                   analytic.theta[static_cast<std::size_t>(edge)])
            << '\n';
    }

    probe << "reference.canonical_materialized_damped_f64.operation_order=materialized_damped_coordinates_float32_promoted_float64_then_theta_float64_dot_plus_centered_bias_float64_then_cast_float32\n";
    probe << "reference.canonical_materialized_damped_f64.prediction_digest="
          << canonical_prediction_digest << '\n';
    probe << "reference.materialized_direct_f32.cpu.prediction_digest="
          << prediction_digest(direct_materialized_cpu) << '\n';
    probe << "reference.materialized_direct_f32.cuda.prediction_digest="
          << prediction_digest(direct_materialized_cuda) << '\n';
    emit_bridge_parity(
        probe, "parity.canonical_to_materialized_direct_f32_cpu",
        "reference.canonical_materialized_damped_f64",
        "reference.materialized_direct_f32.cpu", canonical_reference,
        direct_materialized_cpu);
    emit_bridge_parity(
        probe, "parity.canonical_to_materialized_direct_f32_cuda",
        "reference.canonical_materialized_damped_f64",
        "reference.materialized_direct_f32.cuda", canonical_reference,
        direct_materialized_cuda);
    probe << "reference.live_centered_mapped_f64.cpu.prediction_digest="
          << prediction_digest(centered_f64_cpu) << '\n';
    probe << "reference.live_centered_mapped_f64.cuda.prediction_digest="
          << prediction_digest(centered_f64_cuda) << '\n';
    emit_bridge_parity(
        probe, "parity.live_centered_f64_cuda_full_to_chunked",
        "reference.live_centered_mapped_f64.cuda.full",
        "reference.live_centered_mapped_f64.cuda.chunked",
        centered_f64_cuda, centered_f64_cuda_chunked);
    emit_bridge_parity(
        probe, "parity.canonical_to_live_centered_f64_cuda",
        "reference.canonical_materialized_damped_f64",
        "reference.live_centered_mapped_f64.cuda", canonical_reference,
        centered_f64_cuda);

    emit_bridge_arm(
        probe, "arm.unsafe_uncentered_raw_f32",
        "readout_prefix_float32_normalize_float32_then_uncentered_mapped_weight_dot_float32_plus_uncentered_standardized_bias_float32",
        unsafe_raw_cuda, canonical_reference, centered_f64_cuda);
    emit_bridge_arm(
        probe, "arm.live_centered_mapped_f32",
        "readout_prefix_float32_normalize_float32_then_subtract_edge_center_float32_then_mapped_weight_dot_float32_plus_centered_bias_float32",
        centered_f32_cuda, canonical_reference, centered_f64_cuda);
    emit_bridge_arm(
        probe, "arm.live_explicit_damped_f32",
        "readout_prefix_float32_normalize_float32_then_subtract_edge_center_float32_then_damped_transform_float32_then_theta_dot_float32_plus_centered_bias_float32",
        explicit_damped_cuda, canonical_reference, centered_f64_cuda);

    probe << "artifact.family=" << metadata.artifact_family << '\n';
    probe << "artifact.schema_version=" << metadata.schema_version << '\n';
    probe << "artifact.run_only=" << bool_text(metadata.run_only) << '\n';
    probe << "artifact.semantic_digest="
          << artifact_source_semantic_digest << '\n';
    probe << "artifact.source.semantic_digest="
          << artifact_source_semantic_digest << '\n';
    probe << "artifact.source.conditioning_transform.schema_id="
          << metadata.conditioning_transform_schema_id << '\n';
    probe << "artifact.source.conditioning_transform.sha256="
          << metadata.conditioning_transform_sha256 << '\n';
    probe << "artifact.reloaded.semantic_digest="
          << reloaded_cpu.metadata.semantic_tensor_digest << '\n';
    probe << "artifact.reloaded.conditioning_transform.schema_id="
          << reloaded_cpu.metadata.conditioning_transform_schema_id << '\n';
    probe << "artifact.reloaded.conditioning_transform.sha256="
          << reloaded_cpu.metadata.conditioning_transform_sha256 << '\n';
    probe << "artifact.reloaded_cpu.semantic_digest="
          << reloaded_cpu.metadata.semantic_tensor_digest << '\n';
    probe << "artifact.reloaded_cuda.semantic_digest="
          << reloaded_cuda.metadata.semantic_tensor_digest << '\n';
    probe << "artifact.in_memory.cpu.prediction_digest="
          << prediction_digest(in_memory_cpu_full) << '\n';
    probe << "artifact.in_memory.cuda.prediction_digest="
          << prediction_digest(in_memory_cuda_full) << '\n';
    probe << "artifact.reloaded_cpu.prediction_digest="
          << prediction_digest(archive_cpu_full) << '\n';
    probe << "artifact.reloaded_cuda.prediction_digest="
          << prediction_digest(archive_cuda_full) << '\n';

    emit_bridge_parity(probe, "parity.cpu_reference_to_in_memory",
                       "reference.canonical_materialized_damped_f64",
                       "artifact.in_memory.cpu", canonical_reference,
                       in_memory_cpu_full);
    emit_bridge_parity(probe, "parity.cpu_to_cuda",
                       "artifact.in_memory.cpu",
                       "artifact.in_memory.cuda", in_memory_cpu_full,
                       in_memory_cuda_full);
    emit_bridge_parity(probe, "parity.cpu_full_to_chunked",
                       "artifact.in_memory.cpu.full",
                       "artifact.in_memory.cpu.chunked", in_memory_cpu_full,
                       in_memory_cpu_chunked);
    emit_bridge_parity(probe, "parity.cuda_full_to_chunked",
                       "artifact.in_memory.cuda.full",
                       "artifact.in_memory.cuda.chunked", in_memory_cuda_full,
                       in_memory_cuda_chunked);
    emit_bridge_parity(probe, "parity.in_memory_to_archive_reload",
                       "artifact.in_memory.cuda",
                       "artifact.reloaded.cuda", in_memory_cuda_full,
                       archive_cuda_full);
    emit_bridge_parity(probe,
                       "parity.cpu_in_memory_to_archive_reload",
                       "artifact.in_memory.cpu", "artifact.reloaded.cpu",
                       in_memory_cpu_full, archive_cpu_full);
    emit_bridge_parity(probe,
                       "parity.archive_cpu_full_to_chunked",
                       "artifact.reloaded.cpu.full",
                       "artifact.reloaded.cpu.chunked", archive_cpu_full,
                       archive_cpu_chunked);
    emit_bridge_parity(probe,
                       "parity.archive_cuda_full_to_chunked",
                       "artifact.reloaded.cuda.full",
                       "artifact.reloaded.cuda.chunked", archive_cuda_full,
                       archive_cuda_chunked);
    emit_bridge_parity(probe,
                       "parity.live_centered_f64_to_in_memory_cpu",
                       "reference.live_centered_mapped_f64.cpu",
                       "artifact.in_memory.cpu", centered_f64_cpu,
                       in_memory_cpu_full);
    emit_bridge_parity(probe,
                       "parity.live_centered_f64_to_in_memory_cuda",
                       "reference.live_centered_mapped_f64.cuda",
                       "artifact.in_memory.cuda", centered_f64_cuda,
                       in_memory_cuda_full);
    emit_bridge_parity(probe, "parity.archive_cpu_suffix_ignored",
                       "artifact.reloaded.cpu.original_suffix",
                       "artifact.reloaded.cpu.changed_suffix",
                       archive_cpu_full, archive_cpu_suffix);
    emit_bridge_parity(probe, "parity.archive_cuda_suffix_ignored",
                       "artifact.reloaded.cuda.original_suffix",
                       "artifact.reloaded.cuda.changed_suffix",
                       archive_cuda_full, archive_cuda_suffix);

    const bool artifact_semantic_match =
        artifact_source_semantic_digest ==
            reloaded_cpu.metadata.semantic_tensor_digest &&
        artifact_source_semantic_digest ==
            reloaded_cuda.metadata.semantic_tensor_digest;
    const bool bridge_pass =
        !unsafe_to_canonical.pass &&
        binding.trained_arm_to_analytic_maximum_abs_delta <=
            kBridgeDeploymentTolerance &&
        parity_cpu_reference_to_in_memory.pass &&
        parity_cpu_to_cuda.pass && parity_cpu_full_to_chunked.torch_equal &&
        parity_cuda_full_to_chunked.torch_equal &&
        parity_in_memory_to_archive_reload.torch_equal &&
        parity_cpu_archive.torch_equal &&
        parity_archive_cpu_chunk.torch_equal &&
        parity_archive_cuda_chunk.torch_equal &&
        parity_live_centered_cpu.torch_equal &&
        parity_live_centered_cuda.torch_equal &&
        parity_canonical_to_centered_cuda.pass &&
        parity_suffix_cpu.torch_equal && parity_suffix_cuda.torch_equal &&
        artifact_semantic_match;
    probe << "conclusion.unsafe_uncentered_raw_f32.pass="
          << bool_text(unsafe_to_canonical.pass)
          << '\n';
    probe << "conclusion.live_centered_mapped_f32.pass="
          << bool_text(
                 bridge_delta(centered_f32_cuda, canonical_reference).pass)
          << '\n';
    probe << "conclusion.live_explicit_damped_f32.pass="
          << bool_text(
                 bridge_delta(explicit_damped_cuda, canonical_reference).pass)
          << '\n';
    probe << "conclusion.live_centered_mapped_f64.pass="
          << bool_text(parity_canonical_to_centered_cuda.pass)
          << '\n';
    probe << "conclusion.artifact_semantic_match="
          << bool_text(artifact_semantic_match) << '\n';
    probe << "conclusion.deployment_bridge_pass=" << bool_text(bridge_pass)
          << '\n';
    probe << "conclusion.result_role=development_only_deployment_arithmetic_diagnostic\n";

    if (!bridge_pass) {
      throw std::runtime_error(
          "deployment bridge gate failed before artifact installation");
    }
    std::error_code rename_error;
    std::filesystem::rename(artifact_temporary, options.artifact_output,
                            rename_error);
    if (rename_error || !std::filesystem::exists(options.artifact_output) ||
        std::filesystem::exists(artifact_temporary)) {
      throw std::runtime_error(
          "failed to install the validated deployment artifact");
    }
    write_probe_atomic(options.output, probe.str());
    artifact_staging.committed = true;
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "deployment bridge failed: " << error.what_without_backtrace()
              << '\n';
    return 1;
  } catch (const std::exception &error) {
    std::cerr << "deployment bridge failed: " << error.what() << '\n';
    return 1;
  }
}
