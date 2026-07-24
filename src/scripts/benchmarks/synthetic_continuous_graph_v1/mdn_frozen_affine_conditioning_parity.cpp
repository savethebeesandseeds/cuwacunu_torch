#define main mdn_frozen_affine_objective_ladder_embedded_main
#include "mdn_frozen_affine_objective_ladder.cpp"
#undef main

#include <array>

#include <ATen/ops/linalg_svd.h>

namespace {

constexpr std::string_view kConditioningSchema =
    "synthetic_mdn_frozen_affine_conditioning_parity_v1";
constexpr double kConditioningAlpha = 1.0e-10;
constexpr double kConditioningTauAbsolute = 1.0e-12;
constexpr double kConditioningTauRelative = 1.0e-10;
constexpr double kConditioningDirectionTolerance = 1.0e-2;
constexpr double kConditioningRankTolerance = 1.0e-2;
constexpr double kConditioningRmseTolerance = 5.0e-4;
constexpr double kConditioningDeploymentTolerance = 2.0e-5;
constexpr int64_t kConditioningLbfgsIterations = 500;
constexpr int64_t kConditioningLbfgsEvaluations = 625;
constexpr int64_t kConditioningFitEnd = 554;

struct ConditioningOptions {
  std::filesystem::path development_input;
  std::filesystem::path oracle_archive;
  std::filesystem::path output;
  std::string schema_id;
};

ConditioningOptions parse_conditioning_options(int argc, char **argv) {
  ConditioningOptions options;
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
    } else if (argument == "--oracle-archive") {
      options.oracle_archive = value("--oracle-archive");
    } else if (argument == "--output") {
      options.output = value("--output");
    } else if (argument == "--schema-id") {
      options.schema_id = value("--schema-id");
    } else {
      throw std::runtime_error("unknown conditioning-parity argument: " +
                               argument);
    }
  }
  if (options.development_input.empty() || options.oracle_archive.empty() ||
      options.output.empty() || options.schema_id.empty()) {
    throw std::runtime_error(
        "--development-input --oracle-archive --output and --schema-id are "
        "required");
  }
  const std::set<std::filesystem::path> inputs{
      normalized_path(options.development_input),
      normalized_path(options.oracle_archive)};
  if (inputs.size() != 2 ||
      inputs.contains(normalized_path(options.output))) {
    throw std::runtime_error(
        "development input, oracle archive, and output must be distinct");
  }
  return options;
}

void append_float64_tensor(std::string &bytes, const std::string &name,
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

std::string conditioning_tensor_digest(const std::string &role,
                                       const torch::Tensor &tensor) {
  std::string bytes =
      "synthetic_mdn_frozen_affine_conditioning_parity_tensor.v1\nrole=" +
      role + "\n";
  append_float64_tensor(bytes, role, tensor);
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

struct ConditioningBand {
  int64_t count{0};
  double effective_dof{0.0};
  double target_coupling_energy{0.0};
  double objective_reduction_contribution{0.0};
};

struct ConditioningGeometry {
  torch::Tensor x_mean;            // [F], CPU float64.
  torch::Tensor u;                 // [N,F], CPU float64, canonical signs.
  torch::Tensor singular_values;   // [F], CPU float64, descending.
  torch::Tensor basis;             // [F,F], CPU float64, canonical signs.
  torch::Tensor eigenvalues;       // [F], CPU float64, descending.
  torch::Tensor retained_basis;    // [F,K], CPU float64.
  torch::Tensor retained_transform; // [F,K], CPU float64, whitened.
  torch::Tensor damped_transform;  // [F,F], CPU float64.
  double lambda_max{0.0};
  double tau{0.0};
  double svd_rank_tolerance{0.0};
  int64_t numerical_rank{0};
  int64_t retained_rank{0};
  ConditioningBand low;
  ConditioningBand middle;
  ConditioningBand high;
  double basis_orthogonality_max_abs_error{0.0};
  double retained_whitened_covariance_max_abs_error{0.0};
  double transform_reconstruction_max_abs_error{0.0};
  double preconditioned_hessian_max_abs_error{0.0};
};

struct ConditioningSurface {
  torch::Tensor normalization_mean;    // [F], exact fit-only CPU float32.
  torch::Tensor normalization_inv_std; // [F], exact fit-only CPU float32.
  torch::Tensor canonical_x; // [A,E,C,F], materialized float32 then float64.
  torch::Tensor target;      // [A,E,C], cached float32 promoted to float64.
  std::vector<ConditioningGeometry> geometry;
  std::vector<torch::Tensor> centered_design; // [A*C,F] per edge.
  std::vector<torch::Tensor> retained_coordinates;
  std::vector<torch::Tensor> damped_coordinates;
};

void canonicalize_svd_signs(torch::Tensor &u, torch::Tensor &basis) {
  torch::NoGradGuard no_grad;
  if (u.dim() != 2 || basis.dim() != 2 ||
      u.size(1) != basis.size(1)) {
    throw std::runtime_error("SVD sign-canonicalization shape mismatch");
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

ConditioningBand summarize_conditioning_band(
    const torch::Tensor &eigenvalues, const torch::Tensor &coupling,
    const torch::Tensor &mask) {
  ConditioningBand band;
  band.count = mask.sum().item<int64_t>();
  if (band.count == 0) {
    return band;
  }
  const auto lambda = eigenvalues.masked_select(mask);
  const auto g = coupling.masked_select(mask);
  band.effective_dof =
      (lambda / (lambda + kConditioningAlpha)).sum().item<double>();
  band.target_coupling_energy = g.pow(2).sum().item<double>();
  band.objective_reduction_contribution =
      (g.pow(2) / (lambda + kConditioningAlpha)).sum().item<double>();
  return band;
}

ConditioningSurface build_conditioning_surface(const Dataset &development) {
  torch::NoGradGuard no_grad;
  ConditioningSurface surface;
  const auto fit_indices = integer_range(0, kConditioningFitEnd);
  const auto normalization =
      compute_normalization(development.dynamic_features, fit_indices);
  surface.normalization_mean =
      normalization.mean.to(torch::kCPU, torch::kFloat32).contiguous();
  surface.normalization_inv_std =
      normalization.inv_std.to(torch::kCPU, torch::kFloat32).contiguous();
  const auto canonical_x_float32 =
      ((development.dynamic_features.to(torch::kCPU, torch::kFloat32) -
        surface.normalization_mean.view({1, 1, 1, -1})) *
       surface.normalization_inv_std.view({1, 1, 1, -1}))
          .contiguous();
  surface.canonical_x =
      canonical_x_float32.to(torch::kFloat64).contiguous();
  surface.target =
      development.target.to(torch::kCPU, torch::kFloat64).contiguous();
  if (surface.canonical_x.sizes() !=
          torch::IntArrayRef({kDevelopmentEnd, kEdgeCount, kChannelCount,
                              kDynamicFeatureCount}) ||
      surface.target.sizes() !=
          torch::IntArrayRef(
              {kDevelopmentEnd, kEdgeCount, kChannelCount})) {
    throw std::runtime_error("conditioning canonical surface shape mismatch");
  }

  const int64_t fit_rows = kConditioningFitEnd * kChannelCount;
  const auto identity = torch::eye(kDynamicFeatureCount, torch::kFloat64);
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto design = surface.canonical_x.select(1, edge)
                            .contiguous()
                            .reshape({kDevelopmentEnd * kChannelCount,
                                      kDynamicFeatureCount});
    const auto fit_design = design.narrow(0, 0, fit_rows);
    const auto x_mean = fit_design.mean(0);
    const auto centered = (design - x_mean).contiguous();
    const auto fit_centered = centered.narrow(0, 0, fit_rows);
    auto svd = at::linalg_svd(fit_centered, false, c10::nullopt);
    auto u = std::get<0>(svd).to(torch::kFloat64).contiguous();
    auto singular = std::get<1>(svd).to(torch::kFloat64).contiguous();
    auto basis =
        std::get<2>(svd).transpose(0, 1).to(torch::kFloat64).contiguous();
    canonicalize_svd_signs(u, basis);
    const auto eigenvalues =
        (singular.pow(2) / static_cast<double>(fit_rows)).contiguous();
    const double lambda_max = eigenvalues.select(0, 0).item<double>();
    const double tau = std::max(kConditioningTauAbsolute,
                                kConditioningTauRelative * lambda_max);
    const auto retained_mask = eigenvalues > tau;
    const int64_t retained_rank = retained_mask.sum().item<int64_t>();
    if (retained_rank <= 0 || retained_rank > kDynamicFeatureCount) {
      throw std::runtime_error("conditioning retained rank is invalid");
    }
    const auto retained_basis = basis.narrow(1, 0, retained_rank).contiguous();
    const auto retained_eigenvalues =
        eigenvalues.narrow(0, 0, retained_rank);
    const auto retained_transform =
        (retained_basis * retained_eigenvalues.rsqrt().view({1, -1}))
            .contiguous();
    const auto damped_scale =
        (eigenvalues + kConditioningAlpha).rsqrt();
    const auto damped_transform =
        (basis * damped_scale.view({1, -1})).contiguous();
    const double rank_tolerance =
        std::numeric_limits<double>::epsilon() *
        static_cast<double>(std::max(fit_rows, kDynamicFeatureCount)) *
        singular.select(0, 0).item<double>();
    const int64_t numerical_rank =
        (singular > rank_tolerance).sum().item<int64_t>();

    const auto target = surface.target.narrow(0, 0, kConditioningFitEnd)
                            .select(1, edge)
                            .contiguous()
                            .reshape({fit_rows});
    const auto centered_target = target - target.mean();
    const auto coupling =
        basis.transpose(0, 1)
            .matmul(fit_centered.transpose(0, 1)
                        .matmul(centered_target.unsqueeze(1)))
            .squeeze(1) /
        static_cast<double>(fit_rows);
    const auto low_mask = eigenvalues <= kConditioningAlpha;
    const auto middle_mask =
        (eigenvalues > kConditioningAlpha) * (eigenvalues <= tau);
    const auto high_mask = eigenvalues > tau;

    const auto orthogonality =
        basis.transpose(0, 1).matmul(basis) - identity;
    const auto retained_whitened =
        retained_transform.transpose(0, 1)
            .matmul(fit_centered.transpose(0, 1).matmul(fit_centered) /
                    static_cast<double>(fit_rows))
            .matmul(retained_transform) -
        torch::eye(retained_rank, torch::kFloat64);
    const auto damped_coordinates = centered.matmul(damped_transform);
    const auto reconstructed =
        damped_coordinates
            .matmul((basis *
                     (eigenvalues + kConditioningAlpha)
                         .sqrt()
                         .view({1, -1}))
                        .transpose(0, 1));
    const auto covariance =
        fit_centered.transpose(0, 1).matmul(fit_centered) /
        static_cast<double>(fit_rows);
    const auto preconditioned =
        damped_transform.transpose(0, 1)
            .matmul(covariance + kConditioningAlpha * identity)
            .matmul(damped_transform) -
        identity;

    ConditioningGeometry geometry;
    geometry.x_mean = x_mean.contiguous();
    geometry.u = u;
    geometry.singular_values = singular;
    geometry.basis = basis;
    geometry.eigenvalues = eigenvalues;
    geometry.retained_basis = retained_basis;
    geometry.retained_transform = retained_transform;
    geometry.damped_transform = damped_transform;
    geometry.lambda_max = lambda_max;
    geometry.tau = tau;
    geometry.svd_rank_tolerance = rank_tolerance;
    geometry.numerical_rank = numerical_rank;
    geometry.retained_rank = retained_rank;
    geometry.low =
        summarize_conditioning_band(eigenvalues, coupling, low_mask);
    geometry.middle =
        summarize_conditioning_band(eigenvalues, coupling, middle_mask);
    geometry.high =
        summarize_conditioning_band(eigenvalues, coupling, high_mask);
    geometry.basis_orthogonality_max_abs_error =
        orthogonality.abs().max().item<double>();
    geometry.retained_whitened_covariance_max_abs_error =
        retained_whitened.abs().max().item<double>();
    geometry.transform_reconstruction_max_abs_error =
        (reconstructed - centered).abs().max().item<double>();
    geometry.preconditioned_hessian_max_abs_error =
        preconditioned.abs().max().item<double>();
    surface.geometry.push_back(std::move(geometry));
    surface.centered_design.push_back(centered);
    surface.retained_coordinates.push_back(
        centered.matmul(retained_transform).contiguous());
    surface.damped_coordinates.push_back(damped_coordinates.contiguous());
  }
  return surface;
}

struct ConditioningAnalytic {
  std::vector<torch::Tensor> parameter;
  torch::Tensor mapped_weights; // [E,F], CPU float64.
  torch::Tensor centered_bias;  // [E], CPU float64.
  torch::Tensor uncentered_standardized_bias; // [E], CPU float64.
  torch::Tensor prediction;     // [A,E,C], CPU float32.
  double data_mse{0.0};
  double ridge_penalty{0.0};
  double objective{0.0};
  double runtime_prediction_quantization_max_abs{0.0};
  double runtime_prediction_quantization_mean_abs{0.0};
  double optimizer_surface_kkt_l2_norm{0.0};
  double optimizer_surface_kkt_max_abs{0.0};
  double full_canonical_surface_kkt_l2_norm{0.0};
  double full_canonical_surface_kkt_max_abs{0.0};
  PredictionComparison direct_to_mapped_deployment_comparison;
  bool direct_to_mapped_deployment_pass{false};
};

ConditioningAnalytic solve_analytic_for_transforms(
    const ConditioningSurface &surface,
    const std::vector<torch::Tensor> &coordinates,
    const std::vector<torch::Tensor> &transforms) {
  if (coordinates.size() != static_cast<std::size_t>(kEdgeCount) ||
      transforms.size() != static_cast<std::size_t>(kEdgeCount)) {
    throw std::runtime_error("analytic transform count mismatch");
  }
  torch::NoGradGuard no_grad;
  const int64_t fit_rows = kConditioningFitEnd * kChannelCount;
  auto mapped_weights =
      torch::zeros({kEdgeCount, kDynamicFeatureCount}, torch::kFloat64);
  auto centered_bias = torch::zeros({kEdgeCount}, torch::kFloat64);
  std::vector<torch::Tensor> parameters;
  std::vector<torch::Tensor> edge_predictions;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto &coordinate = coordinates[static_cast<std::size_t>(edge)];
    const auto &transform = transforms[static_cast<std::size_t>(edge)];
    if (coordinate.size(0) != kDevelopmentEnd * kChannelCount ||
        coordinate.size(1) != transform.size(1) ||
        transform.size(0) != kDynamicFeatureCount) {
      throw std::runtime_error("analytic coordinate/transform shape mismatch");
    }
    const auto z = coordinate.narrow(0, 0, fit_rows);
    const auto z_mean = z.mean(0);
    const auto z_centered = z - z_mean;
    const auto y = surface.target.narrow(0, 0, kConditioningFitEnd)
                       .select(1, edge)
                       .contiguous()
                       .reshape({fit_rows});
    const auto y_mean = y.mean();
    const auto y_centered = y - y_mean;
    const auto augmented_design = torch::cat(
        {z_centered,
         std::sqrt(static_cast<double>(fit_rows) * kConditioningAlpha) *
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
    canonicalize_svd_signs(u, v);
    const double tolerance =
        std::numeric_limits<double>::epsilon() *
        static_cast<double>(std::max(augmented_design.size(0),
                                     augmented_design.size(1))) *
        singular.select(0, 0).item<double>();
    if ((singular <= tolerance).any().item<bool>()) {
      throw std::runtime_error("augmented analytic ridge system lost rank");
    }
    const auto parameter =
        v.matmul((u.transpose(0, 1)
                      .matmul(augmented_target.unsqueeze(1))
                      .squeeze(1)) /
                 singular);
    const auto mapped = transform.matmul(parameter);
    const auto intercept = y_mean - z_mean.dot(parameter);
    parameters.push_back(parameter.contiguous());
    mapped_weights.select(0, edge).copy_(mapped);
    centered_bias.select(0, edge).copy_(intercept);
    edge_predictions.push_back(
        (coordinate.matmul(parameter) + intercept)
            .reshape({kDevelopmentEnd, kChannelCount}));
  }
  const auto exact_prediction =
      torch::stack(edge_predictions, 1).to(torch::kFloat64).contiguous();
  const auto prediction = exact_prediction.to(torch::kFloat32).contiguous();
  const auto prediction_quantization =
      (exact_prediction - prediction.to(torch::kFloat64)).abs();
  auto uncentered_standardized_bias = centered_bias.clone();
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    uncentered_standardized_bias.select(0, edge).sub_(
        surface.geometry[static_cast<std::size_t>(edge)].x_mean.dot(
            mapped_weights.select(0, edge)));
  }
  const auto fit_prediction =
      exact_prediction.narrow(0, 0, kConditioningFitEnd);
  const auto fit_target = surface.target.narrow(0, 0, kConditioningFitEnd);
  const double data_mse =
      (fit_prediction - fit_target).pow(2).mean().item<double>();
  const double ridge_penalty =
      kConditioningAlpha / static_cast<double>(kEdgeCount) *
      mapped_weights.pow(2).sum().item<double>();

  double kkt_sq = 0.0;
  double kkt_max = 0.0;
  double optimizer_kkt_sq = 0.0;
  double optimizer_kkt_max = 0.0;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto x = surface.centered_design[static_cast<std::size_t>(edge)]
                       .narrow(0, 0, fit_rows);
    const auto y = surface.target.narrow(0, 0, kConditioningFitEnd)
                       .select(1, edge)
                       .contiguous()
                       .reshape({fit_rows});
    const auto residual =
        x.matmul(mapped_weights.select(0, edge)) +
        centered_bias.select(0, edge) - y;
    const auto grad_w =
        (2.0 / static_cast<double>(fit_rows) *
             x.transpose(0, 1).matmul(residual.unsqueeze(1)).squeeze(1) +
         2.0 * kConditioningAlpha * mapped_weights.select(0, edge)) /
        static_cast<double>(kEdgeCount);
    const auto grad_b =
        2.0 * residual.mean() / static_cast<double>(kEdgeCount);
    kkt_sq += grad_w.pow(2).sum().item<double>() +
              grad_b.pow(2).item<double>();
    kkt_max = std::max(
        kkt_max,
        std::max(grad_w.abs().max().item<double>(),
                 std::fabs(grad_b.item<double>())));

    const auto &coordinate = coordinates[static_cast<std::size_t>(edge)];
    const auto &transform = transforms[static_cast<std::size_t>(edge)];
    const auto z = coordinate.narrow(0, 0, fit_rows);
    const auto optimizer_residual =
        z.matmul(parameters[static_cast<std::size_t>(edge)]) +
        centered_bias.select(0, edge) - y;
    const auto optimizer_grad =
        (2.0 / static_cast<double>(fit_rows) *
             z.transpose(0, 1)
                 .matmul(optimizer_residual.unsqueeze(1))
                 .squeeze(1) +
         2.0 * kConditioningAlpha *
             transform.transpose(0, 1).matmul(transform.matmul(
                 parameters[static_cast<std::size_t>(edge)]))) /
        static_cast<double>(kEdgeCount);
    const auto optimizer_grad_bias =
        2.0 * optimizer_residual.mean() /
        static_cast<double>(kEdgeCount);
    optimizer_kkt_sq +=
        optimizer_grad.pow(2).sum().item<double>() +
        optimizer_grad_bias.pow(2).item<double>();
    optimizer_kkt_max = std::max(
        optimizer_kkt_max,
        std::max(optimizer_grad.abs().max().item<double>(),
                 std::fabs(optimizer_grad_bias.item<double>())));
  }
  return {.parameter = std::move(parameters),
          .mapped_weights = mapped_weights,
          .centered_bias = centered_bias,
          .uncentered_standardized_bias = uncentered_standardized_bias,
          .prediction = prediction,
          .data_mse = data_mse,
          .ridge_penalty = ridge_penalty,
          .objective = data_mse + ridge_penalty,
          .runtime_prediction_quantization_max_abs =
              prediction_quantization.max().item<double>(),
          .runtime_prediction_quantization_mean_abs =
              prediction_quantization.mean().item<double>(),
          .optimizer_surface_kkt_l2_norm = std::sqrt(optimizer_kkt_sq),
          .optimizer_surface_kkt_max_abs = optimizer_kkt_max,
          .full_canonical_surface_kkt_l2_norm = std::sqrt(kkt_sq),
          .full_canonical_surface_kkt_max_abs = kkt_max,
          .direct_to_mapped_deployment_comparison = {},
          .direct_to_mapped_deployment_pass = false};
}

std::vector<torch::Tensor> identity_transforms() {
  std::vector<torch::Tensor> transforms;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    transforms.push_back(
        torch::eye(kDynamicFeatureCount, torch::kFloat64));
  }
  return transforms;
}

std::vector<torch::Tensor> retained_transforms(
    const ConditioningSurface &surface) {
  std::vector<torch::Tensor> transforms;
  for (const auto &geometry : surface.geometry) {
    transforms.push_back(geometry.retained_transform);
  }
  return transforms;
}

std::vector<torch::Tensor> damped_transforms(
    const ConditioningSurface &surface) {
  std::vector<torch::Tensor> transforms;
  for (const auto &geometry : surface.geometry) {
    transforms.push_back(geometry.damped_transform);
  }
  return transforms;
}

std::vector<torch::Tensor> materialize_f32_then_promote(
    const std::vector<torch::Tensor> &values) {
  std::vector<torch::Tensor> result;
  result.reserve(values.size());
  for (const auto &value : values) {
    result.push_back(
        value.to(torch::kFloat32).to(torch::kFloat64).contiguous());
  }
  return result;
}

struct ConditioningMappedModelImpl : torch::nn::Module {
  std::vector<torch::Tensor> transforms;
  std::vector<torch::Tensor> weights;
  torch::Tensor bias;

  explicit ConditioningMappedModelImpl(
      const std::vector<torch::Tensor> &cpu_transforms) {
    if (cpu_transforms.size() != static_cast<std::size_t>(kEdgeCount)) {
      throw std::runtime_error("mapped model transform count mismatch");
    }
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      const auto transform = cpu_transforms[static_cast<std::size_t>(edge)]
                                 .to(torch::kFloat32)
                                 .contiguous();
      if (transform.dim() != 2 ||
          transform.size(0) != kDynamicFeatureCount ||
          transform.size(1) <= 0) {
        throw std::runtime_error("mapped model transform shape mismatch");
      }
      transforms.push_back(register_buffer(
          "transform_" + std::to_string(edge), transform.clone()));
      weights.push_back(register_parameter(
          "weight_" + std::to_string(edge),
          torch::zeros({transform.size(1)}, torch::kFloat32)));
    }
    bias = register_parameter("bias",
                              torch::zeros({kEdgeCount}, torch::kFloat32));
  }

  torch::Tensor forward(const std::vector<torch::Tensor> &coordinates,
                        int64_t anchor_count) {
    if (coordinates.size() != static_cast<std::size_t>(kEdgeCount)) {
      throw std::runtime_error("mapped model coordinate count mismatch");
    }
    std::vector<torch::Tensor> edges;
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      const auto &coordinate = coordinates[static_cast<std::size_t>(edge)];
      if (coordinate.size(0) != anchor_count * kChannelCount ||
          coordinate.size(1) != weights[static_cast<std::size_t>(edge)].size(0)) {
        throw std::runtime_error("mapped model coordinate shape mismatch");
      }
      edges.push_back(
          (coordinate.matmul(weights[static_cast<std::size_t>(edge)]) +
           bias.select(0, edge))
              .reshape({anchor_count, kChannelCount}));
    }
    return torch::stack(edges, 1);
  }

  torch::Tensor ridge_penalty() const {
    auto sum = bias.new_zeros({});
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      const auto mapped =
          transforms[static_cast<std::size_t>(edge)].matmul(
              weights[static_cast<std::size_t>(edge)]);
      sum = sum + mapped.pow(2).sum();
    }
    return kConditioningAlpha / static_cast<double>(kEdgeCount) * sum;
  }

  torch::Tensor mapped_weights_cpu64() const {
    torch::NoGradGuard no_grad;
    std::vector<torch::Tensor> rows;
    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      rows.push_back(transforms[static_cast<std::size_t>(edge)]
                         .matmul(weights[static_cast<std::size_t>(edge)])
                         .to(torch::kCPU, torch::kFloat64));
    }
    return torch::stack(rows, 0).contiguous();
  }
};

using ConditioningMappedModel =
    std::shared_ptr<ConditioningMappedModelImpl>;

struct ConditioningObjective {
  torch::Tensor total;
  torch::Tensor data_mse;
  torch::Tensor ridge_penalty;
};

ConditioningObjective conditioning_objective(
    const ConditioningMappedModel &model,
    const std::vector<torch::Tensor> &coordinates, int64_t anchor_count,
    const torch::Tensor &target) {
  const auto prediction = model->forward(coordinates, anchor_count);
  const auto data_mse = (prediction - target).pow(2).mean();
  const auto ridge_penalty = model->ridge_penalty();
  return {data_mse + ridge_penalty, data_mse, ridge_penalty};
}

double conditioning_gradient_norm(const ConditioningMappedModel &model) {
  double squared = 0.0;
  for (const auto &parameter : model->parameters()) {
    if (parameter.grad().defined()) {
      squared += parameter.grad().detach().pow(2).sum().item<double>();
    }
  }
  return std::sqrt(squared);
}

std::vector<torch::Tensor> conditioning_domain_coordinates(
    const std::vector<torch::Tensor> &development_coordinates,
    int64_t anchor_begin, int64_t anchor_end, const torch::Device &device) {
  if (anchor_begin < 0 || anchor_end <= anchor_begin ||
      anchor_end > kDevelopmentEnd) {
    throw std::runtime_error("invalid conditioning coordinate domain");
  }
  std::vector<torch::Tensor> result;
  for (const auto &coordinate : development_coordinates) {
    result.push_back(
        coordinate
            .narrow(0, anchor_begin * kChannelCount,
                    (anchor_end - anchor_begin) * kChannelCount)
            .to(device, torch::kFloat32)
            .contiguous());
  }
  return result;
}

struct ConditioningGate {
  double direction_shortfall{0.0};
  double rank_shortfall{0.0};
  double rmse_excess{0.0};
  bool pass{false};
};

ConditioningGate conditioning_gate(const Metric &candidate,
                                   const Metric &reference) {
  ConditioningGate gate;
  gate.direction_shortfall =
      std::max(0.0, metric_direction(reference) -
                        metric_direction(candidate));
  gate.rank_shortfall =
      std::max(0.0, metric_rank(reference) - metric_rank(candidate));
  gate.rmse_excess =
      std::max(0.0, metric_rmse(candidate) - metric_rmse(reference));
  gate.pass = gate.direction_shortfall <= kConditioningDirectionTolerance &&
              gate.rank_shortfall <= kConditioningRankTolerance &&
              gate.rmse_excess <= kConditioningRmseTolerance;
  return gate;
}

struct ConditioningPointwiseDelta {
  bool torch_equal{false};
  double maximum_abs_delta{0.0};
  double mean_abs_delta{0.0};
  double rmse_delta{0.0};
};

ConditioningPointwiseDelta conditioning_pointwise_delta(
    const torch::Tensor &lhs_input, const torch::Tensor &rhs_input) {
  const auto lhs =
      lhs_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  const auto rhs =
      rhs_input.detach().to(torch::kCPU, torch::kFloat32).contiguous();
  if (lhs.sizes() != rhs.sizes() ||
      !torch::isfinite(lhs).all().item<bool>() ||
      !torch::isfinite(rhs).all().item<bool>()) {
    throw std::runtime_error("pointwise reference delta domain is invalid");
  }
  const auto delta = lhs - rhs;
  return {.torch_equal = torch::equal(lhs, rhs),
          .maximum_abs_delta = delta.abs().max().item<double>(),
          .mean_abs_delta = delta.abs().mean().item<double>(),
          .rmse_delta = delta.pow(2).mean().sqrt().item<double>()};
}

void emit_conditioning_pointwise_delta(
    std::ostream &output, const std::string &prefix,
    const std::string &lhs, const std::string &rhs,
    const std::string &scope, const ConditioningPointwiseDelta &delta) {
  output << prefix << ".lhs=" << lhs << '\n';
  output << prefix << ".rhs=" << rhs << '\n';
  output << prefix << ".scope=" << scope << '\n';
  output << prefix << ".torch_equal=" << bool_text(delta.torch_equal)
         << '\n';
  output << prefix << ".maximum_abs_delta=" << delta.maximum_abs_delta
         << '\n';
  output << prefix << ".mean_abs_delta=" << delta.mean_abs_delta << '\n';
  output << prefix << ".rmse_delta=" << delta.rmse_delta << '\n';
}

struct ConditioningArm {
  std::string optimizer;
  int64_t configured_steps{0};
  int64_t iterations{0};
  int64_t closure_evaluations{0};
  double initial_objective{0.0};
  double final_data_mse{0.0};
  double final_ridge_penalty{0.0};
  double final_objective{0.0};
  double cpu_promoted_data_mse{0.0};
  double cpu_promoted_ridge_penalty{0.0};
  double cpu_promoted_objective{0.0};
  double cuda_to_cpu_promoted_objective_delta{0.0};
  double initial_gradient_l2_norm{0.0};
  double final_gradient_l2_norm{0.0};
  double full_canonical_surface_kkt_l2_norm{0.0};
  double full_canonical_surface_kkt_max_abs{0.0};
  double parameter_update_l2_norm{0.0};
  double optimizer_surface_relative_objective_gap{0.0};
  torch::Tensor mapped_weights;
  torch::Tensor uncentered_standardized_bias;
  torch::Tensor prediction;
  Metric fit_metric;
  Metric validation_metric;
  ConditioningGate scientific_gate;
  std::string scientific_gate_reference;
  std::string optimizer_surface_reference;
  PredictionComparison analytic_prediction_comparison;
  PredictionComparison r0_prediction_comparison;
  PredictionComparison deployment_comparison;
  bool deployment_pass{false};
  double mapped_intercept_cancellation_max_abs{0.0};
  double mapped_intercept_cancellation_ratio_max{0.0};
  std::string state_digest;
  std::string prediction_digest;
};

std::string conditioning_state_digest(const torch::Tensor &weights,
                                      const torch::Tensor &bias) {
  std::string bytes =
      "synthetic_mdn_frozen_affine_conditioning_parity_state.v1\n";
  append_float64_tensor(bytes, "mapped_weights", weights);
  append_float64_tensor(bytes, "uncentered_standardized_bias", bias);
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

std::pair<double, double> full_canonical_surface_kkt(
    const ConditioningSurface &surface, const torch::Tensor &weights,
    const torch::Tensor &centered_bias) {
  torch::NoGradGuard no_grad;
  const int64_t fit_rows = kConditioningFitEnd * kChannelCount;
  double squared = 0.0;
  double maximum = 0.0;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto x = surface.centered_design[static_cast<std::size_t>(edge)]
                       .narrow(0, 0, fit_rows);
    const auto y = surface.target.narrow(0, 0, kConditioningFitEnd)
                       .select(1, edge)
                       .contiguous()
                       .reshape({fit_rows});
    const auto residual = x.matmul(weights.select(0, edge)) +
                          centered_bias.select(0, edge) - y;
    const auto grad_w =
        (2.0 / static_cast<double>(fit_rows) *
             x.transpose(0, 1).matmul(residual.unsqueeze(1)).squeeze(1) +
         2.0 * kConditioningAlpha * weights.select(0, edge)) /
        static_cast<double>(kEdgeCount);
    const auto grad_b =
        2.0 * residual.mean() / static_cast<double>(kEdgeCount);
    squared += grad_w.pow(2).sum().item<double>() +
               grad_b.pow(2).item<double>();
    maximum = std::max(
        maximum,
        std::max(grad_w.abs().max().item<double>(),
                 std::fabs(grad_b.item<double>())));
  }
  return {std::sqrt(squared), maximum};
}

torch::Tensor mapped_prediction_float32(
    const ConditioningSurface &surface, const torch::Tensor &weights,
    const torch::Tensor &uncentered_standardized_bias,
    const torch::Device &device) {
  torch::NoGradGuard no_grad;
  const auto features = surface.canonical_x.to(device, torch::kFloat32);
  const auto mapped_weights = weights.to(device, torch::kFloat32);
  const auto mapped_bias =
      uncentered_standardized_bias.to(device, torch::kFloat32);
  return (features * mapped_weights.view(
                         {1, kEdgeCount, 1, kDynamicFeatureCount}))
             .sum(-1) +
         mapped_bias.view({1, kEdgeCount, 1});
}

void evaluate_analytic_deployment(
    ConditioningAnalytic &analytic, const ConditioningSurface &surface,
    const std::vector<torch::Tensor> &coordinates,
    const torch::Device &device) {
  torch::NoGradGuard no_grad;
  if (coordinates.size() != static_cast<std::size_t>(kEdgeCount) ||
      analytic.parameter.size() != static_cast<std::size_t>(kEdgeCount)) {
    throw std::runtime_error("analytic deployment domain mismatch");
  }
  std::vector<torch::Tensor> edges;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto coordinate = coordinates[static_cast<std::size_t>(edge)]
                                .to(device, torch::kFloat32);
    const auto parameter = analytic.parameter[static_cast<std::size_t>(edge)]
                               .to(device, torch::kFloat32);
    const auto bias =
        analytic.centered_bias.select(0, edge).to(device, torch::kFloat32);
    edges.push_back((coordinate.matmul(parameter) + bias)
                        .reshape({kDevelopmentEnd, kChannelCount}));
  }
  const auto direct_prediction = torch::stack(edges, 1);
  const auto mapped_prediction = mapped_prediction_float32(
      surface, analytic.mapped_weights,
      analytic.uncentered_standardized_bias, device);
  analytic.direct_to_mapped_deployment_comparison =
      compare_predictions(direct_prediction, mapped_prediction);
  analytic.direct_to_mapped_deployment_pass =
      analytic.direct_to_mapped_deployment_comparison.maximum_abs_delta <=
      kConditioningDeploymentTolerance;
}

ConditioningArm train_conditioning_arm(
    const std::string &optimizer_name, int64_t adam_steps,
    const ConditioningSurface &surface,
    const std::vector<torch::Tensor> &development_coordinates,
    const std::vector<torch::Tensor> &transforms,
    const ConditioningAnalytic &analytic_reference,
    const ConditioningAnalytic &r0_reference, const Metric &gate_reference,
    const std::string &gate_reference_name,
    const std::string &optimizer_surface_reference_name,
    const torch::Device &device) {
  reset_deterministic_seed();
  auto model = std::make_shared<ConditioningMappedModelImpl>(transforms);
  model->to(device, torch::kFloat32);
  const auto fit_coordinates = conditioning_domain_coordinates(
      development_coordinates, 0, kConditioningFitEnd, device);
  const auto validation_coordinates = conditioning_domain_coordinates(
      development_coordinates, kPurgeEnd, kDevelopmentEnd, device);
  const auto all_coordinates = conditioning_domain_coordinates(
      development_coordinates, 0, kDevelopmentEnd, device);
  const auto fit_target = surface.target.narrow(0, 0, kConditioningFitEnd)
                              .to(device, torch::kFloat32);
  const auto validation_target =
      surface.target.narrow(0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd)
          .to(device, torch::kFloat32);

  ConditioningArm result;
  result.optimizer = optimizer_name;
  result.scientific_gate_reference = gate_reference_name;
  result.optimizer_surface_reference = optimizer_surface_reference_name;
  result.configured_steps = optimizer_name == "adam" ? adam_steps : 1;
  auto initial = conditioning_objective(model, fit_coordinates,
                                        kConditioningFitEnd, fit_target);
  result.initial_objective = initial.total.item<double>();
  initial.total.backward();
  result.initial_gradient_l2_norm = conditioning_gradient_norm(model);
  model->zero_grad();
  std::vector<torch::Tensor> initial_parameters;
  for (const auto &parameter : model->parameters()) {
    initial_parameters.push_back(parameter.detach().clone());
  }

  if (optimizer_name == "adam") {
    torch::optim::AdamOptions options(kLearningRate);
    options.betas(std::make_tuple(0.9, 0.999));
    options.eps(1.0e-8);
    options.weight_decay(0.0);
    options.amsgrad(false);
    torch::optim::Adam optimizer(model->parameters(), options);
    for (int64_t step = 0; step < adam_steps; ++step) {
      optimizer.zero_grad();
      const auto objective = conditioning_objective(
          model, fit_coordinates, kConditioningFitEnd, fit_target);
      if (!torch::isfinite(objective.total).item<bool>()) {
        throw std::runtime_error("conditioning Adam objective is non-finite");
      }
      objective.total.backward();
      optimizer.step();
    }
    result.iterations = adam_steps;
  } else if (optimizer_name == "lbfgs_strong_wolfe") {
    torch::optim::LBFGSOptions options(1.0);
    options.max_iter(kConditioningLbfgsIterations);
    options.max_eval(kConditioningLbfgsEvaluations);
    options.tolerance_grad(1.0e-12);
    options.tolerance_change(1.0e-15);
    options.history_size(100);
    options.line_search_fn(std::optional<std::string>{"strong_wolfe"});
    torch::optim::LBFGS optimizer(model->parameters(), options);
    auto closure = [&]() {
      optimizer.zero_grad();
      const auto objective = conditioning_objective(
          model, fit_coordinates, kConditioningFitEnd, fit_target);
      if (!torch::isfinite(objective.total).item<bool>()) {
        throw std::runtime_error("conditioning LBFGS objective is non-finite");
      }
      objective.total.backward();
      ++result.closure_evaluations;
      return objective.total;
    };
    optimizer.step(closure);
    result.iterations = kConditioningLbfgsIterations;
    if (!optimizer.state().empty()) {
      const auto *state = dynamic_cast<const torch::optim::LBFGSParamState *>(
          optimizer.state().begin()->second.get());
      if (state != nullptr) {
        result.iterations = state->n_iter();
      }
    }
  } else {
    throw std::runtime_error("unknown conditioning optimizer");
  }

  double update_squared = 0.0;
  const auto final_parameters = model->parameters();
  for (std::size_t index = 0; index < initial_parameters.size(); ++index) {
    update_squared +=
        (final_parameters[index].detach() - initial_parameters[index])
            .pow(2)
            .sum()
            .item<double>();
  }
  result.parameter_update_l2_norm = std::sqrt(update_squared);
  model->zero_grad();
  const auto final = conditioning_objective(model, fit_coordinates,
                                            kConditioningFitEnd, fit_target);
  result.final_data_mse = final.data_mse.item<double>();
  result.final_ridge_penalty = final.ridge_penalty.item<double>();
  result.final_objective = final.total.item<double>();
  final.total.backward();
  result.final_gradient_l2_norm = conditioning_gradient_norm(model);
  model->zero_grad();

  torch::NoGradGuard no_grad;
  std::vector<torch::Tensor> cpu_fit_predictions;
  double cpu_penalty_sum = 0.0;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const auto parameter =
        model->weights[static_cast<std::size_t>(edge)]
            .detach()
            .to(torch::kCPU, torch::kFloat64);
    const auto bias =
        model->bias.select(0, edge).detach().to(torch::kCPU, torch::kFloat64);
    const auto fit_coordinate =
        development_coordinates[static_cast<std::size_t>(edge)].narrow(
            0, 0, kConditioningFitEnd * kChannelCount);
    cpu_fit_predictions.push_back(
        (fit_coordinate.matmul(parameter) + bias)
            .reshape({kConditioningFitEnd, kChannelCount}));
    const auto mapped =
        transforms[static_cast<std::size_t>(edge)].matmul(parameter);
    cpu_penalty_sum += mapped.pow(2).sum().item<double>();
  }
  const auto cpu_fit_prediction = torch::stack(cpu_fit_predictions, 1);
  result.cpu_promoted_data_mse =
      (cpu_fit_prediction -
       surface.target.narrow(0, 0, kConditioningFitEnd))
          .pow(2)
          .mean()
          .item<double>();
  result.cpu_promoted_ridge_penalty =
      kConditioningAlpha / static_cast<double>(kEdgeCount) *
      cpu_penalty_sum;
  result.cpu_promoted_objective =
      result.cpu_promoted_data_mse + result.cpu_promoted_ridge_penalty;
  result.cuda_to_cpu_promoted_objective_delta =
      result.final_objective - result.cpu_promoted_objective;
  result.optimizer_surface_relative_objective_gap =
      (result.cpu_promoted_objective - analytic_reference.objective) /
      std::max(std::fabs(analytic_reference.objective), 1.0e-30);
  result.prediction =
      model->forward(all_coordinates, kDevelopmentEnd).detach().to(torch::kCPU);
  const auto fit_prediction =
      result.prediction.narrow(0, 0, kConditioningFitEnd);
  const auto validation_prediction = result.prediction.narrow(
      0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd);
  result.fit_metric = evaluate_prediction(
      fit_prediction, fit_target.to(torch::kCPU));
  result.validation_metric = evaluate_prediction(
      validation_prediction, validation_target.to(torch::kCPU));
  result.scientific_gate =
      conditioning_gate(result.validation_metric, gate_reference);
  result.analytic_prediction_comparison = compare_predictions(
      validation_prediction,
      analytic_reference.prediction.narrow(
          0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd));
  result.r0_prediction_comparison = compare_predictions(
      validation_prediction,
      r0_reference.prediction.narrow(0, kPurgeEnd,
                                     kDevelopmentEnd - kPurgeEnd));
  result.mapped_weights = model->mapped_weights_cpu64();
  auto centered_bias = model->bias.detach().to(torch::kCPU, torch::kFloat64);
  result.uncentered_standardized_bias = centered_bias.clone();
  double cancellation_max = 0.0;
  double cancellation_ratio = 0.0;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    const double correction =
        surface.geometry[static_cast<std::size_t>(edge)].x_mean.dot(
            result.mapped_weights.select(0, edge))
            .item<double>();
    const double centered = centered_bias.select(0, edge).item<double>();
    result.uncentered_standardized_bias.select(0, edge).sub_(correction);
    cancellation_max = std::max(cancellation_max, std::fabs(correction));
    cancellation_ratio = std::max(
        cancellation_ratio,
        (std::fabs(centered) + std::fabs(correction)) /
            std::max(std::fabs(centered - correction), 1.0e-30));
  }
  result.mapped_intercept_cancellation_max_abs = cancellation_max;
  result.mapped_intercept_cancellation_ratio_max = cancellation_ratio;
  const auto kkt = full_canonical_surface_kkt(
      surface, result.mapped_weights, centered_bias);
  result.full_canonical_surface_kkt_l2_norm = kkt.first;
  result.full_canonical_surface_kkt_max_abs = kkt.second;
  const auto deployment_prediction = mapped_prediction_float32(
      surface, result.mapped_weights,
      result.uncentered_standardized_bias, device);
  result.deployment_comparison =
      compare_predictions(result.prediction, deployment_prediction);
  result.deployment_pass =
      result.deployment_comparison.maximum_abs_delta <=
      kConditioningDeploymentTolerance;
  result.state_digest =
      conditioning_state_digest(result.mapped_weights,
                                result.uncentered_standardized_bias);
  result.prediction_digest = ::prediction_digest(result.prediction);
  return result;
}

void emit_conditioning_band(std::ostream &output, const std::string &prefix,
                            const ConditioningBand &band) {
  output << prefix << ".count=" << band.count << '\n';
  output << prefix << ".effective_dof=" << band.effective_dof << '\n';
  output << prefix << ".target_coupling_energy="
         << band.target_coupling_energy << '\n';
  output << prefix << ".objective_reduction_contribution="
         << band.objective_reduction_contribution << '\n';
}

void emit_conditioning_analytic(std::ostream &output,
                                const std::string &prefix,
                                const ConditioningAnalytic &analytic,
                                const ConditioningSurface &surface) {
  const auto fit_metric = evaluate_prediction(
      analytic.prediction.narrow(0, 0, kConditioningFitEnd),
      surface.target.narrow(0, 0, kConditioningFitEnd));
  const auto validation_metric = evaluate_prediction(
      analytic.prediction.narrow(0, kPurgeEnd,
                                 kDevelopmentEnd - kPurgeEnd),
      surface.target.narrow(0, kPurgeEnd,
                            kDevelopmentEnd - kPurgeEnd));
  output << prefix << ".fit_scope=[0,554)\n";
  output << prefix << ".validation_scope=[584,730)\n";
  output << prefix << ".objective.data_mse=" << analytic.data_mse << '\n';
  output << prefix << ".objective.ridge_penalty="
         << analytic.ridge_penalty << '\n';
  output << prefix << ".objective.total=" << analytic.objective << '\n';
  output << prefix << ".objective.evaluation_dtype=float64\n";
  output << prefix << ".runtime_prediction.dtype=float32\n";
  output << prefix << ".runtime_prediction.quantization_max_abs="
         << analytic.runtime_prediction_quantization_max_abs << '\n';
  output << prefix << ".runtime_prediction.quantization_mean_abs="
         << analytic.runtime_prediction_quantization_mean_abs << '\n';
  output << prefix << ".optimizer_surface_kkt.l2_norm="
         << analytic.optimizer_surface_kkt_l2_norm << '\n';
  output << prefix << ".optimizer_surface_kkt.max_abs="
         << analytic.optimizer_surface_kkt_max_abs << '\n';
  output << prefix << ".full_canonical_surface_kkt.l2_norm="
         << analytic.full_canonical_surface_kkt_l2_norm << '\n';
  output << prefix << ".full_canonical_surface_kkt.max_abs="
         << analytic.full_canonical_surface_kkt_max_abs << '\n';
  output << prefix << ".mapped_weight_l2_norm="
         << analytic.mapped_weights.norm().item<double>() << '\n';
  output << prefix << ".state_digest="
         << conditioning_state_digest(analytic.mapped_weights,
                                      analytic.uncentered_standardized_bias)
         << '\n';
  output << prefix << ".prediction_digest="
         << ::prediction_digest(analytic.prediction) << '\n';
  emit_comparison(
      output, prefix +
                  ".deployment_parity.direct_coordinate_to_mapped_delta",
      analytic.direct_to_mapped_deployment_comparison);
  output << prefix << ".deployment_parity.max_abs_tolerance="
         << kConditioningDeploymentTolerance << '\n';
  output << prefix << ".deployment_parity.pass="
         << bool_text(analytic.direct_to_mapped_deployment_pass) << '\n';
  emit_metric(output, prefix + ".fit", fit_metric);
  emit_metric(output, prefix + ".validation", validation_metric);
}

void emit_conditioning_arm(std::ostream &output, const std::string &prefix,
                           const ConditioningArm &arm) {
  output << prefix << ".optimizer=" << arm.optimizer << '\n';
  output << prefix << ".configured_steps=" << arm.configured_steps << '\n';
  output << prefix << ".iterations=" << arm.iterations << '\n';
  output << prefix << ".closure_evaluations=" << arm.closure_evaluations
         << '\n';
  output << prefix << ".objective.initial=" << arm.initial_objective << '\n';
  output << prefix << ".objective.cuda_f32.data_mse="
         << arm.final_data_mse << '\n';
  output << prefix << ".objective.cuda_f32.ridge_penalty="
         << arm.final_ridge_penalty << '\n';
  output << prefix << ".objective.cuda_f32.total=" << arm.final_objective
         << '\n';
  output << prefix << ".objective.cpu_f64_promoted_parameters.data_mse="
         << arm.cpu_promoted_data_mse << '\n';
  output
      << prefix
      << ".objective.cpu_f64_promoted_parameters.ridge_penalty="
      << arm.cpu_promoted_ridge_penalty << '\n';
  output << prefix << ".objective.cpu_f64_promoted_parameters.total="
         << arm.cpu_promoted_objective << '\n';
  output << prefix
         << ".objective.cuda_f32_to_cpu_f64_promoted_delta="
         << arm.cuda_to_cpu_promoted_objective_delta << '\n';
  output << prefix
         << ".optimizer_surface_kkt.initial_cuda_f32_autograd_l2_norm="
         << arm.initial_gradient_l2_norm << '\n';
  output << prefix
         << ".optimizer_surface_kkt.final_cuda_f32_autograd_l2_norm="
         << arm.final_gradient_l2_norm << '\n';
  output << prefix << ".full_canonical_surface_kkt.l2_norm="
         << arm.full_canonical_surface_kkt_l2_norm << '\n';
  output << prefix << ".full_canonical_surface_kkt.max_abs="
         << arm.full_canonical_surface_kkt_max_abs << '\n';
  output << prefix << ".parameter_update_l2_norm="
         << arm.parameter_update_l2_norm << '\n';
  output << prefix << ".optimizer_surface_relative_objective_gap="
         << arm.optimizer_surface_relative_objective_gap << '\n';
  output << prefix << ".optimizer_surface_relative_objective_gap.reference="
         << arm.optimizer_surface_reference << '\n';
  output << prefix << ".mapped_weight_l2_norm="
         << arm.mapped_weights.norm().item<double>() << '\n';
  output << prefix << ".mapped_intercept_cancellation_max_abs="
         << arm.mapped_intercept_cancellation_max_abs << '\n';
  output << prefix << ".mapped_intercept_cancellation_ratio_max="
         << arm.mapped_intercept_cancellation_ratio_max << '\n';
  output << prefix << ".state_digest=" << arm.state_digest << '\n';
  output << prefix << ".prediction_digest=" << arm.prediction_digest << '\n';
  emit_metric(output, prefix + ".fit", arm.fit_metric);
  emit_metric(output, prefix + ".validation", arm.validation_metric);
  output << prefix << ".scientific_gate.direction_shortfall="
         << arm.scientific_gate.direction_shortfall << '\n';
  output << prefix << ".scientific_gate.rank_shortfall="
         << arm.scientific_gate.rank_shortfall << '\n';
  output << prefix << ".scientific_gate.rmse_excess="
         << arm.scientific_gate.rmse_excess << '\n';
  output << prefix << ".scientific_gate.pass="
         << bool_text(arm.scientific_gate.pass) << '\n';
  output << prefix << ".scientific_gate.reference="
         << arm.scientific_gate_reference << '\n';
  emit_comparison(output, prefix + ".analytic_prediction_delta",
                  arm.analytic_prediction_comparison);
  output << prefix << ".analytic_prediction_delta.reference="
         << arm.optimizer_surface_reference << '\n';
  emit_comparison(output, prefix + ".R0_prediction_delta",
                  arm.r0_prediction_comparison);
  emit_comparison(output, prefix + ".deployment_parity.prediction_delta",
                  arm.deployment_comparison);
  output << prefix << ".deployment_parity.max_abs_tolerance="
         << kConditioningDeploymentTolerance << '\n';
  output << prefix << ".deployment_parity.pass="
         << bool_text(arm.deployment_pass) << '\n';
}

} // namespace

int main(int argc, char **argv) {
  try {
    std::locale::global(std::locale::classic());
    const auto options = parse_conditioning_options(argc, argv);
    if (options.schema_id != kConditioningSchema) {
      throw std::runtime_error(
          "conditioning-parity schema does not match the contract");
    }
    const auto temporary =
        std::filesystem::path(options.output.string() + ".tmp");
    if (std::filesystem::exists(options.output) ||
        std::filesystem::exists(temporary)) {
      throw std::runtime_error(
          "refusing to replace a conditioning result or temporary");
    }

    const auto runtime = configure_deterministic_runtime();
    const torch::Device device(torch::kCUDA, 0);
    const auto development_sha256 = sha256_file(options.development_input);
    const auto oracle_archive_sha256 = sha256_file(options.oracle_archive);
    const auto development =
        read_probe(options.development_input, 0, kDevelopmentEnd);
    if (development.record_schema != kProbeRecordSchema) {
      throw std::runtime_error("development probe schema is not frozen");
    }
    auto loaded_sidecar =
        mdn::load_per_edge_affine_return_head(options.oracle_archive);
    validate_sidecar_binding(loaded_sidecar, development, development_sha256);
    if (!sidecar_parameters_frozen(loaded_sidecar.head)) {
      throw std::runtime_error("external sidecar parameters are not frozen");
    }

    const auto context_fit_indices = integer_range(0, kConditioningFitEnd);
    const auto context_fit_analytic = fit_analytic_ridge(
        development, context_fit_indices, kConditioningAlpha);
    const auto context_fit_prediction = predict_state_from_context(
        context_fit_analytic.state, development.context);
    const auto context_fit_metric = evaluate_prediction(
        context_fit_prediction.narrow(0, 0, kConditioningFitEnd),
        development.target.narrow(0, 0, kConditioningFitEnd));
    const auto context_validation_metric = evaluate_prediction(
        context_fit_prediction.narrow(
            0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
        development.target.narrow(0, kPurgeEnd,
                                  kDevelopmentEnd - kPurgeEnd));

    const auto surface = build_conditioning_surface(development);
    const auto ordinary_transforms = identity_transforms();
    const auto retained = retained_transforms(surface);
    const auto damped = damped_transforms(surface);
    auto full_analytic = solve_analytic_for_transforms(
        surface, surface.centered_design, ordinary_transforms);
    auto r0_analytic = solve_analytic_for_transforms(
        surface, surface.retained_coordinates, retained);
    evaluate_analytic_deployment(full_analytic, surface,
                                 surface.centered_design, device);
    evaluate_analytic_deployment(r0_analytic, surface,
                                 surface.retained_coordinates, device);
    const auto retained_coordinates_f32 =
        materialize_f32_then_promote(surface.retained_coordinates);
    const auto retained_transforms_f32 =
        materialize_f32_then_promote(retained);
    auto r1_runtime_analytic = solve_analytic_for_transforms(
        surface, retained_coordinates_f32, retained_transforms_f32);
    evaluate_analytic_deployment(r1_runtime_analytic, surface,
                                 retained_coordinates_f32, device);
    const auto r0_reference_metric = evaluate_prediction(
        r0_analytic.prediction.narrow(
            0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
        surface.target.narrow(0, kPurgeEnd,
                              kDevelopmentEnd - kPurgeEnd));

    const auto r1_adam = train_conditioning_arm(
        "adam", kSteps, surface, retained_coordinates_f32,
        retained_transforms_f32, r1_runtime_analytic, r0_analytic,
        r0_reference_metric, "reference.R0_retained_analytic",
        "reference.R1_materialized_f32_analytic", device);
    const auto r1_lbfgs = train_conditioning_arm(
        "lbfgs_strong_wolfe", 0, surface, retained_coordinates_f32,
        retained_transforms_f32, r1_runtime_analytic, r0_analytic,
        r0_reference_metric, "reference.R0_retained_analytic",
        "reference.R1_materialized_f32_analytic", device);
    const auto full_reference_metric = evaluate_prediction(
        full_analytic.prediction.narrow(
            0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
        surface.target.narrow(0, kPurgeEnd,
                              kDevelopmentEnd - kPurgeEnd));
    const auto full_context_fit_comparison = compare_predictions(
        full_analytic.prediction.narrow(
            0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
        context_fit_prediction.narrow(
            0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd));
    const auto r0_operational_gate = conditioning_gate(
        evaluate_prediction(
            r0_analytic.prediction.narrow(
                0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
            surface.target.narrow(0, kPurgeEnd,
                                  kDevelopmentEnd - kPurgeEnd)),
        full_reference_metric);
    const auto r0_full_validation_comparison = compare_predictions(
        r0_analytic.prediction.narrow(
            0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
        full_analytic.prediction.narrow(
            0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd));
    const auto ordinary_coordinates_f32 =
        materialize_f32_then_promote(surface.centered_design);
    const auto ordinary_transforms_f32 =
        materialize_f32_then_promote(ordinary_transforms);
    auto r2_ordinary_runtime_analytic = solve_analytic_for_transforms(
        surface, ordinary_coordinates_f32, ordinary_transforms_f32);
    const auto damped_coordinates_f32 =
        materialize_f32_then_promote(surface.damped_coordinates);
    const auto damped_transforms_f32 = materialize_f32_then_promote(damped);
    auto r2_damped_runtime_analytic = solve_analytic_for_transforms(
        surface, damped_coordinates_f32, damped_transforms_f32);
    evaluate_analytic_deployment(r2_ordinary_runtime_analytic, surface,
                                 ordinary_coordinates_f32, device);
    evaluate_analytic_deployment(r2_damped_runtime_analytic, surface,
                                 damped_coordinates_f32, device);
    const auto r1_r0_development_delta = conditioning_pointwise_delta(
        r1_runtime_analytic.prediction, r0_analytic.prediction);
    const auto r1_r0_validation_delta = conditioning_pointwise_delta(
        r1_runtime_analytic.prediction.narrow(
            0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
        r0_analytic.prediction.narrow(
            0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd));
    const auto r2_ordinary_full_development_delta =
        conditioning_pointwise_delta(r2_ordinary_runtime_analytic.prediction,
                                     full_analytic.prediction);
    const auto r2_ordinary_full_validation_delta =
        conditioning_pointwise_delta(
            r2_ordinary_runtime_analytic.prediction.narrow(
                0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
            full_analytic.prediction.narrow(
                0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd));
    const auto r2_damped_full_development_delta =
        conditioning_pointwise_delta(r2_damped_runtime_analytic.prediction,
                                     full_analytic.prediction);
    const auto r2_damped_full_validation_delta =
        conditioning_pointwise_delta(
            r2_damped_runtime_analytic.prediction.narrow(
                0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
            full_analytic.prediction.narrow(
                0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd));
    const auto r2_ordinary = train_conditioning_arm(
        "lbfgs_strong_wolfe", 0, surface, ordinary_coordinates_f32,
        ordinary_transforms_f32, r2_ordinary_runtime_analytic, r0_analytic,
        full_reference_metric, "reference.full_analytic",
        "reference.R2_ordinary_materialized_f32_analytic", device);
    const auto r2_damped = train_conditioning_arm(
        "lbfgs_strong_wolfe", 0, surface, damped_coordinates_f32,
        damped_transforms_f32, r2_damped_runtime_analytic, r0_analytic,
        full_reference_metric, "reference.full_analytic",
        "reference.R2_damped_materialized_f32_analytic", device);
    require_cuda_success(cudaDeviceSynchronize(), "cudaDeviceSynchronize");

    torch::Tensor external_sidecar_prediction;
    {
      torch::NoGradGuard no_grad;
      loaded_sidecar.head->eval();
      external_sidecar_prediction =
          loaded_sidecar.head->forward(development.context);
    }
    const auto external_sidecar_validation_metric = evaluate_prediction(
        external_sidecar_prediction.narrow(
            0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd),
        development.target.narrow(0, kPurgeEnd,
                                  kDevelopmentEnd - kPurgeEnd));

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
    probe << "input.external_sidecar_archive.sha256="
          << oracle_archive_sha256 << '\n';
    probe << "data.record_schema=" << development.record_schema << '\n';
    probe << "data.canonical_design=cached_384_float32_dynamic_features_exact_fit_only_shared_float32_normalization_materialized_float32_then_promoted_to_cpu_float64\n";
    probe << "data.normalization.scope=[0,554)\n";
    probe << "data.normalization.accumulation=float32\n";
    probe << "data.normalization.mean_digest="
          << conditioning_tensor_digest("normalization_mean",
                                        surface.normalization_mean)
          << '\n';
    probe << "data.normalization.inv_std_digest="
          << conditioning_tensor_digest("normalization_inv_std",
                                        surface.normalization_inv_std)
          << '\n';
    probe << "data.canonical_design_digest="
          << conditioning_tensor_digest("canonical_design",
                                        surface.canonical_x)
          << '\n';
    probe << "data.target_digest="
          << conditioning_tensor_digest("target", surface.target) << '\n';
    probe << "data.maximum_anchor_loaded=729\n";
    probe << "boundary.development_only=true\n";
    probe << "boundary.maximum_anchor_loaded=729\n";
    probe << "boundary.unconsumed_holdout_opened=false\n";
    probe << "split.fit=[0,554)\n";
    probe << "split.purge=[554,584)\n";
    probe << "split.validation=[584,730)\n";
    probe << "split.holdout.opened=false\n";
    probe << "execution.alpha=" << kConditioningAlpha << '\n';
    probe << "execution.ridge_objective=data_mse_plus_alpha_div_edge_count_times_mapped_weight_squared_norm\n";
    probe << "execution.bias_penalized=false\n";
    probe << "execution.adam.steps=" << kSteps << '\n';
    probe << "execution.adam.learning_rate=" << kLearningRate << '\n';
    probe << "execution.lbfgs.max_iterations="
          << kConditioningLbfgsIterations << '\n';
    probe << "execution.lbfgs.max_evaluations="
          << kConditioningLbfgsEvaluations << '\n';
    probe << "execution.lbfgs.line_search=strong_wolfe\n";
    probe << "execution.scientific_gate.direction_shortfall_max="
          << kConditioningDirectionTolerance << '\n';
    probe << "execution.scientific_gate.rank_shortfall_max="
          << kConditioningRankTolerance << '\n';
    probe << "execution.scientific_gate.rmse_excess_max="
          << kConditioningRmseTolerance << '\n';
    probe << "execution.deployment_gate.separate_from_scientific_gate=true\n";
    probe << "execution.deployment_gate.scope=[0,730)\n";
    probe << "execution.deployment_gate.prediction_max_abs_delta="
          << kConditioningDeploymentTolerance << '\n';
    probe << "execution.full_canonical_surface_kkt.role=full_canonical_standardized_coordinate_gradient_residual_including_omitted_modes_and_materialization_error\n";
    probe << "execution.optimizer_surface_kkt.role=direct_parameter_coordinate_gradient_of_data_mse_plus_alpha_div_edge_count_mapped_weight_penalty\n";
    probe << "external_sidecar.semantic_tensor_digest="
          << loaded_sidecar.metadata.semantic_tensor_digest << '\n';
    probe << "external_sidecar.parameters_frozen=true\n";
    probe << "external_sidecar.fit_scope=[0,730)\n";
    probe << "external_sidecar.role=development_0_730_fitted_descriptive_not_validation_reference\n";
    probe << "external_sidecar.used_as_gate_reference=false\n";
    probe << "external_sidecar.validation_prediction_digest="
          << ::prediction_digest(external_sidecar_prediction.narrow(
                 0, kPurgeEnd, kDevelopmentEnd - kPurgeEnd))
          << '\n';
    emit_metric(probe, "external_sidecar.validation",
                external_sidecar_validation_metric);

    probe << "reference.context_recomputed_fit_analytic.fit_scope=[0,554)\n";
    probe << "reference.context_recomputed_fit_analytic.validation_scope=[584,730)\n";
    probe << "reference.context_recomputed_fit_analytic.feature_surface=context_float32_promoted_to_float64_then_base_quote_difference_recomputed\n";
    probe << "reference.context_recomputed_fit_analytic.maximum_normalized_residual="
          << context_fit_analytic.maximum_normalized_residual << '\n';
    probe << "reference.context_recomputed_fit_analytic.coefficient_l2_norm="
          << context_fit_analytic.coefficient_l2_norm << '\n';
    probe << "reference.context_recomputed_fit_analytic.state_digest="
          << state_digest(context_fit_analytic.state) << '\n';
    probe << "reference.context_recomputed_fit_analytic.prediction_digest="
          << ::prediction_digest(context_fit_prediction) << '\n';
    emit_metric(probe, "reference.context_recomputed_fit_analytic.fit",
                context_fit_metric);
    emit_metric(probe,
                "reference.context_recomputed_fit_analytic.validation",
                context_validation_metric);
    emit_comparison(
        probe,
        "reference.full_analytic_to_context_recomputed_fit_analytic.validation",
        full_context_fit_comparison);

    for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
      const auto &geometry =
          surface.geometry[static_cast<std::size_t>(edge)];
      const std::string prefix = "conditioning.edge." + std::to_string(edge);
      probe << prefix << ".lambda_max=" << geometry.lambda_max << '\n';
      probe << prefix << ".tau=" << geometry.tau << '\n';
      probe << prefix << ".svd_rank_tolerance="
            << geometry.svd_rank_tolerance << '\n';
      probe << prefix << ".numerical_rank=" << geometry.numerical_rank
            << '\n';
      probe << prefix << ".retained_rank=" << geometry.retained_rank << '\n';
      probe << prefix << ".basis_orthogonality_max_abs_error="
            << geometry.basis_orthogonality_max_abs_error << '\n';
      probe << prefix << ".retained_whitened_covariance_max_abs_error="
            << geometry.retained_whitened_covariance_max_abs_error << '\n';
      probe << prefix << ".transform_reconstruction_max_abs_error="
            << geometry.transform_reconstruction_max_abs_error << '\n';
      probe << prefix << ".preconditioned_hessian_max_abs_error="
            << geometry.preconditioned_hessian_max_abs_error << '\n';
      probe << prefix << ".basis_digest="
            << conditioning_tensor_digest("basis", geometry.basis) << '\n';
      probe << prefix << ".eigenvalue_digest="
            << conditioning_tensor_digest("eigenvalues",
                                          geometry.eigenvalues)
            << '\n';
      probe << prefix << ".retained_design_digest="
            << conditioning_tensor_digest(
                   "retained_design",
                   surface.retained_coordinates[static_cast<std::size_t>(
                       edge)])
            << '\n';
      probe << prefix << ".retained_transform_digest="
            << conditioning_tensor_digest("retained_transform",
                                          geometry.retained_transform)
            << '\n';
      probe << prefix << ".damped_design_digest="
            << conditioning_tensor_digest(
                   "damped_design",
                   surface.damped_coordinates[static_cast<std::size_t>(edge)])
            << '\n';
      emit_conditioning_band(probe, prefix + ".eigenband.le_alpha",
                             geometry.low);
      emit_conditioning_band(probe, prefix + ".eigenband.alpha_to_tau",
                             geometry.middle);
      emit_conditioning_band(probe, prefix + ".eigenband.gt_tau",
                             geometry.high);
    }

    emit_conditioning_analytic(probe, "reference.full_analytic",
                               full_analytic, surface);
    emit_conditioning_analytic(probe, "reference.R0_retained_analytic",
                               r0_analytic, surface);
    probe << "reference.R0_retained_analytic.operational_gate.reference=reference.full_analytic\n";
    probe << "reference.R0_retained_analytic.operational_gate.direction_shortfall="
          << r0_operational_gate.direction_shortfall << '\n';
    probe << "reference.R0_retained_analytic.operational_gate.rank_shortfall="
          << r0_operational_gate.rank_shortfall << '\n';
    probe << "reference.R0_retained_analytic.operational_gate.rmse_excess="
          << r0_operational_gate.rmse_excess << '\n';
    probe << "reference.R0_retained_analytic.operational_gate.pass="
          << bool_text(r0_operational_gate.pass) << '\n';
    emit_comparison(
        probe,
        "reference.R0_retained_analytic.operational_gate.validation_prediction_delta",
        r0_full_validation_comparison);
    emit_conditioning_analytic(
        probe, "reference.R1_materialized_f32_analytic",
        r1_runtime_analytic, surface);
    emit_conditioning_analytic(
        probe, "reference.R2_ordinary_materialized_f32_analytic",
        r2_ordinary_runtime_analytic, surface);
    emit_conditioning_analytic(
        probe, "reference.R2_damped_materialized_f32_analytic",
        r2_damped_runtime_analytic, surface);
    emit_conditioning_pointwise_delta(
        probe, "reference_delta.R1_materialized_to_R0.development",
        "reference.R1_materialized_f32_analytic",
        "reference.R0_retained_analytic", "[0,730)",
        r1_r0_development_delta);
    emit_conditioning_pointwise_delta(
        probe, "reference_delta.R1_materialized_to_R0.validation",
        "reference.R1_materialized_f32_analytic",
        "reference.R0_retained_analytic", "[584,730)",
        r1_r0_validation_delta);
    emit_conditioning_pointwise_delta(
        probe, "reference_delta.R2_ordinary_materialized_to_full.development",
        "reference.R2_ordinary_materialized_f32_analytic",
        "reference.full_analytic", "[0,730)",
        r2_ordinary_full_development_delta);
    emit_conditioning_pointwise_delta(
        probe, "reference_delta.R2_ordinary_materialized_to_full.validation",
        "reference.R2_ordinary_materialized_f32_analytic",
        "reference.full_analytic", "[584,730)",
        r2_ordinary_full_validation_delta);
    emit_conditioning_pointwise_delta(
        probe, "reference_delta.R2_damped_materialized_to_full.development",
        "reference.R2_damped_materialized_f32_analytic",
        "reference.full_analytic", "[0,730)",
        r2_damped_full_development_delta);
    emit_conditioning_pointwise_delta(
        probe, "reference_delta.R2_damped_materialized_to_full.validation",
        "reference.R2_damped_materialized_f32_analytic",
        "reference.full_analytic", "[584,730)",
        r2_damped_full_validation_delta);
    emit_conditioning_arm(probe, "arm.R1_adam", r1_adam);
    emit_conditioning_arm(probe, "arm.R1_lbfgs", r1_lbfgs);
    emit_conditioning_arm(probe, "arm.R2_ordinary_lbfgs", r2_ordinary);
    emit_conditioning_arm(probe, "arm.R2_damped_lbfgs", r2_damped);
    probe << "conclusion.result_role=development_conditioning_diagnostic\n";
    write_probe_atomic(options.output, probe.str());
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "mdn_frozen_affine_conditioning_parity: " << error.what()
              << '\n';
    return 1;
  }
}
