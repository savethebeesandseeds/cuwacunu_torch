// Development-only frozen MTF pre-pool domain-by-scale affine evaluator.
//
// The canonical probe parser, metric implementation, ridge grid, gates, and
// validation comparator are included byte-for-byte from the frozen Phase 2A
// authority. This translation unit changes only the accepted representation
// width (256 node/channel; 768 base/quote/difference row), retains nine
// independent edge-by-channel heads, and caches ridge-invariant systems so the
// six declared ridge candidates remain bounded without changing their math.
#define main cuwacunu_embedded_pooled_affine_probe_main
#include "frozen_representation_affine_probe.cpp"
#undef main

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace {

constexpr std::string_view kPrepoolSchema =
    "synthetic_v2_frozen_mtf_prepool_domain_scale_channel_conditioned_"
    "affine_development_v1";
constexpr std::string_view kPrepoolProbeKind = "prepool_domain_scale";
constexpr std::string_view kPhase2AAuthoritySha =
    "5103e594a6096a325ac33b115594a739a0c3e3f0ad8d36b9fcf38d8ac8114570";
constexpr std::string_view kParserAuthoritySha =
    "45242804d0a84a074e621ed81ef4336d93f36046ab67a1e6ce23e452d56ac939";
constexpr int64_t kPrepoolRepresentationWidth = 256;
constexpr int64_t kPrepoolAffineWidth = 768;
constexpr int64_t kConditionedHeadCount = kEdgeCount * kChannelCount;
constexpr int64_t kExpectedRidgeFitCount =
    static_cast<int64_t>(kRidgeGrid.size());
constexpr int64_t kMaximumCholeskyCount =
    kConditionedHeadCount * kExpectedRidgeFitCount;

constexpr ProbeSpec kPrepoolSpec{
    .kind = ProbeKind::kRepresentation,
    .name = kPrepoolProbeKind,
    .report_schema = kPrepoolSchema,
    .record_schema = "kikijyeba.synthetic.representation_edge_feature_probe.v1",
    .representation_width = kPrepoolRepresentationWidth,
    .affine_width = kPrepoolAffineWidth,
    .probe_width = kPrepoolAffineWidth};

struct PrepoolModel {
  torch::Tensor mean;    // [D], train core across all edges and channels.
  torch::Tensor inv_std; // [D], train core across all edges and channels.
  torch::Tensor weights; // [E,C,D], one independent row per edge x channel.
  torch::Tensor bias;    // [E,C], one independent bias per edge x channel.
  int64_t feature_width{0};
  double ridge{0.0};
  double maximum_normalized_residual{0.0};
  double coefficient_l2_norm{0.0};
};

struct PrepoolCandidate {
  bool numerically_valid{false};
  double declared_ridge{0.0};
  std::string rejection_reason;
  PrepoolModel model;
  MetricSummary validation;
};

struct RidgeInvariantHeadSystem {
  torch::Tensor x_mean; // [D].
  torch::Tensor y_mean; // scalar.
  torch::Tensor gram;   // [D,D], before ridge diagonal addition.
  torch::Tensor rhs;    // [D,1].
};

struct PrepoolFitWorkspace {
  torch::Tensor mean;
  torch::Tensor inv_std;
  std::array<RidgeInvariantHeadSystem, kConditionedHeadCount> systems{};
  int64_t feature_width{0};
};

struct SolverCounters {
  int64_t ridge_invariant_system_build_count{0};
  int64_t ridge_fit_attempt_count{0};
  int64_t cholesky_factorization_count{0};
  int64_t cholesky_solve_count{0};
};

void write_exclusive_report(const std::filesystem::path &path,
                            const std::string &contents) {
  const int descriptor =
      ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
  if (descriptor < 0) {
    throw std::runtime_error(
        "output report path must be absent and exclusively creatable: " +
        path.string() + ": " + std::strerror(errno));
  }

  int open_descriptor = descriptor;
  try {
    std::size_t offset = 0;
    while (offset < contents.size()) {
      const auto written = ::write(open_descriptor, contents.data() + offset,
                                   contents.size() - offset);
      if (written < 0 && errno == EINTR) {
        continue;
      }
      if (written <= 0) {
        throw std::runtime_error(
            "failed while writing exclusive output report: " +
            std::string(std::strerror(errno)));
      }
      offset += static_cast<std::size_t>(written);
    }
    if (::close(open_descriptor) != 0) {
      open_descriptor = -1;
      throw std::runtime_error(
          "failed while closing exclusive output report: " +
          std::string(std::strerror(errno)));
    }
    open_descriptor = -1;
  } catch (...) {
    if (open_descriptor >= 0) {
      (void)::close(open_descriptor);
    }
    (void)::unlink(path.c_str());
    throw;
  }
}

Options parse_prepool_options(int argc, char **argv) {
  std::vector<std::string> normalized;
  normalized.reserve(static_cast<std::size_t>(argc));
  normalized.emplace_back(argv[0]);
  int probe_kind_count = 0;
  int development_only_count = 0;
  int train_input_count = 0;
  int validation_input_count = 0;
  int output_count = 0;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    normalized.push_back(argument);
    if (argument == "--development-only") {
      ++development_only_count;
      continue;
    }
    int *count = nullptr;
    if (argument == "--probe-kind") {
      count = &probe_kind_count;
    } else if (argument == "--train-input") {
      count = &train_input_count;
    } else if (argument == "--validation-input") {
      count = &validation_input_count;
    } else if (argument == "--output") {
      count = &output_count;
    } else {
      throw std::runtime_error("unsupported pre-pool argument: " + argument);
    }
    ++(*count);
    if (*count != 1 || index + 1 >= argc) {
      throw std::runtime_error(argument +
                               " must appear exactly once with a value");
    }
    const std::string value = argv[++index];
    if (argument == "--probe-kind") {
      if (value != kPrepoolProbeKind) {
        throw std::runtime_error("invalid --probe-kind: " + value);
      }
      // Route through the frozen parser without teaching it a new grammar
      // token; replace its selected ProbeSpec immediately after parsing.
      normalized.emplace_back("representation");
    } else {
      normalized.push_back(value);
    }
  }
  if (probe_kind_count != 1 || development_only_count != 1 ||
      train_input_count != 1 || validation_input_count != 1 ||
      output_count != 1) {
    throw std::runtime_error(
        "exactly one each of --probe-kind prepool_domain_scale, "
        "--development-only, --train-input, --validation-input, and --output "
        "is required");
  }

  std::vector<char *> normalized_argv;
  normalized_argv.reserve(normalized.size());
  for (auto &argument : normalized) {
    normalized_argv.push_back(argument.data());
  }
  auto options = parse_options(static_cast<int>(normalized_argv.size()),
                               normalized_argv.data());
  if (options.probe_spec != &kRepresentationSpec) {
    throw std::runtime_error("frozen parser did not select representation");
  }
  const std::array<std::filesystem::path, 3> paths{
      options.train_input, options.validation_input, options.output};
  std::set<std::filesystem::path> normalized_paths;
  for (const auto &path : paths) {
    if (!path.is_absolute() || path.empty() ||
        path.string().find('\n') != std::string::npos ||
        path.string().find('\r') != std::string::npos ||
        !normalized_paths.insert(path.lexically_normal()).second) {
      throw std::runtime_error(
          "train, validation, and output paths must be absolute, distinct, "
          "and newline-free");
    }
  }
  options.probe_spec = &kPrepoolSpec;
  return options;
}

torch::Tensor prepool_predict(const PrepoolModel &model,
                              const torch::Tensor &features) {
  torch::NoGradGuard no_grad;
  const auto feature_width = model.feature_width;
  if (feature_width != kPrepoolAffineWidth || features.dim() != 4 ||
      features.size(1) != kEdgeCount || features.size(2) != kChannelCount ||
      features.size(3) != feature_width) {
    throw std::runtime_error("pre-pool model and feature shapes differ");
  }
  const auto standardized =
      (features - model.mean.view({1, 1, 1, feature_width})) *
      model.inv_std.view({1, 1, 1, feature_width});
  return (standardized *
          model.weights.view({1, kEdgeCount, kChannelCount, feature_width}))
             .sum(-1) +
         model.bias.view({1, kEdgeCount, kChannelCount});
}

PrepoolFitWorkspace build_fit_workspace(const Dataset &dataset,
                                        SolverCounters *counters) {
  torch::NoGradGuard no_grad;
  if (counters == nullptr || dataset.anchor_begin != kTrainBegin ||
      dataset.anchor_end != kTrainEnd) {
    throw std::runtime_error("invalid pre-pool fit workspace request");
  }
  const auto features = dataset.features;
  const auto target = dataset.target;
  if (features.dim() != 4 || target.dim() != 3 ||
      features.size(0) != kTrainEnd - kTrainBegin ||
      features.size(1) != kEdgeCount || features.size(2) != kChannelCount ||
      features.size(3) != kPrepoolAffineWidth ||
      target.sizes() != torch::IntArrayRef({kTrainEnd - kTrainBegin, kEdgeCount,
                                            kChannelCount})) {
    throw std::runtime_error("pre-pool fit tensor contract mismatch");
  }

  const auto feature_width = features.size(3);
  const auto flat = features.reshape({-1, feature_width});
  const auto mean = flat.mean(0);
  const auto variance = (flat - mean).pow(2).mean(0);
  const auto standard_deviation = variance.sqrt();
  const auto inv_std = torch::where(standard_deviation > 1.0e-12,
                                    standard_deviation.reciprocal(),
                                    torch::ones_like(standard_deviation));
  const auto standardized = (features - mean.view({1, 1, 1, feature_width})) *
                            inv_std.view({1, 1, 1, feature_width});

  PrepoolFitWorkspace workspace{};
  workspace.mean = mean.contiguous();
  workspace.inv_std = inv_std.contiguous();
  workspace.feature_width = feature_width;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    for (int64_t channel = 0; channel < kChannelCount; ++channel) {
      const auto x =
          standardized.select(1, edge).select(1, channel).contiguous();
      const auto y = target.select(1, edge).select(1, channel).contiguous();
      if (x.sizes() !=
              torch::IntArrayRef({kTrainEnd - kTrainBegin, feature_width}) ||
          y.sizes() != torch::IntArrayRef({kTrainEnd - kTrainBegin})) {
        throw std::runtime_error(
            "edge-channel fitting slice has an unexpected shape");
      }
      const auto x_mean = x.mean(0);
      const auto y_mean = y.mean();
      const auto centered_x = x - x_mean;
      const auto centered_y = y - y_mean;
      const auto head = edge * kChannelCount + channel;
      workspace.systems[static_cast<std::size_t>(head)] = {
          .x_mean = x_mean.contiguous(),
          .y_mean = y_mean,
          .gram = centered_x.transpose(0, 1).matmul(centered_x).contiguous(),
          .rhs = centered_x.transpose(0, 1)
                     .matmul(centered_y.unsqueeze(1))
                     .contiguous()};
      ++counters->ridge_invariant_system_build_count;
    }
  }
  if (counters->ridge_invariant_system_build_count != kConditionedHeadCount) {
    throw std::runtime_error("ridge-invariant system count mismatch");
  }
  return workspace;
}

PrepoolModel fit_prepool(const PrepoolFitWorkspace &workspace, double ridge,
                         SolverCounters *counters) {
  if (counters == nullptr || !(ridge > 0.0) || !std::isfinite(ridge) ||
      workspace.feature_width != kPrepoolAffineWidth) {
    throw std::runtime_error("invalid pre-pool ridge fit request");
  }
  torch::NoGradGuard no_grad;
  ++counters->ridge_fit_attempt_count;
  auto weights = torch::zeros(
      {kEdgeCount, kChannelCount, workspace.feature_width}, torch::kFloat64);
  auto bias = torch::zeros({kEdgeCount, kChannelCount}, torch::kFloat64);
  double maximum_residual = 0.0;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    for (int64_t channel = 0; channel < kChannelCount; ++channel) {
      const auto head = edge * kChannelCount + channel;
      const auto &system = workspace.systems[static_cast<std::size_t>(head)];
      auto gram = system.gram.clone();
      gram.diagonal(0, 0, 1).add_(static_cast<double>(kTrainEnd - kTrainBegin) *
                                  ridge);
      ++counters->cholesky_factorization_count;
      auto [cholesky, info] = at::linalg_cholesky_ex(gram, false, false);
      if (info.item<int64_t>() != 0) {
        throw CandidateNumericalError("cholesky_factorization_failed");
      }
      const auto row =
          at::cholesky_solve(system.rhs, cholesky, false).squeeze(1);
      ++counters->cholesky_solve_count;
      const auto residual = gram.matmul(row.unsqueeze(1)) - system.rhs;
      const double normalized_residual =
          residual.norm().item<double>() /
          std::max(system.rhs.norm().item<double>(), 1.0e-30);
      if (!std::isfinite(normalized_residual) || normalized_residual > 1.0e-7 ||
          !torch::isfinite(row).all().item<bool>()) {
        throw CandidateNumericalError(
            "normalized_residual_or_finiteness_failed");
      }
      maximum_residual = std::max(maximum_residual, normalized_residual);
      weights.select(0, edge).select(0, channel).copy_(row);
      bias.select(0, edge)
          .select(0, channel)
          .copy_(system.y_mean - system.x_mean.dot(row));
    }
  }
  return {.mean = workspace.mean,
          .inv_std = workspace.inv_std,
          .weights = weights.contiguous(),
          .bias = bias.contiguous(),
          .feature_width = workspace.feature_width,
          .ridge = ridge,
          .maximum_normalized_residual = maximum_residual,
          .coefficient_l2_norm = weights.norm().item<double>()};
}

MetricSummary evaluate_prepool(const Dataset &dataset,
                               const PrepoolModel &model) {
  return summarize(
      observe(prepool_predict(model, dataset.features), dataset.target));
}

MetricSummary evaluate_prepool_channel(const Dataset &dataset,
                                       const PrepoolModel &model,
                                       int64_t channel) {
  if (channel < 0 || channel >= kChannelCount) {
    throw std::runtime_error("invalid channel index");
  }
  return summarize(
      observe(prepool_predict(model, dataset.features).narrow(2, channel, 1),
              dataset.target.narrow(2, channel, 1)));
}

Candidate comparator_view(const PrepoolCandidate &candidate) {
  Candidate view;
  view.numerically_valid = candidate.numerically_valid;
  view.declared_ridge = candidate.declared_ridge;
  view.rejection_reason = candidate.rejection_reason;
  view.model.ridge = candidate.model.ridge;
  view.validation = candidate.validation;
  return view;
}

bool prepool_better(const PrepoolCandidate &candidate,
                    const PrepoolCandidate &incumbent) {
  return better(comparator_view(candidate), comparator_view(incumbent));
}

void run_prepool(const Options &options) {
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  torch::manual_seed(31);

  if (options.probe_spec != &kPrepoolSpec) {
    throw std::runtime_error(
        "pre-pool evaluator accepts only --probe-kind prepool_domain_scale");
  }
  if (!options.development_only || options.validation_only ||
      !options.certified_input.empty() || !options.selection_lock.empty()) {
    throw std::runtime_error(
        "pre-pool evaluator requires --development-only and has no certified, "
        "final, selection-lock, or validation-control mode");
  }

  const auto train_dataset =
      read_probe(options.train_input, kTrainBegin, kTrainEnd, kPrepoolSpec);
  const auto validation_dataset = read_probe(
      options.validation_input, kValidationBegin, kValidationEnd, kPrepoolSpec);
  SolverCounters solver_counters{};
  const auto workspace = build_fit_workspace(train_dataset, &solver_counters);

  std::vector<PrepoolCandidate> candidates;
  candidates.reserve(kRidgeGrid.size());
  for (const double ridge : kRidgeGrid) {
    try {
      auto model = fit_prepool(workspace, ridge, &solver_counters);
      auto validation = evaluate_prepool(validation_dataset, model);
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
    if (selected == candidates.end() || prepool_better(*candidate, *selected)) {
      selected = candidate;
    }
  }
  if (selected == candidates.end()) {
    throw std::runtime_error("no ridge candidate was selected");
  }
  if (solver_counters.ridge_fit_attempt_count != kExpectedRidgeFitCount) {
    throw std::runtime_error("ridge fit attempt counter mismatch");
  }

  const auto train = evaluate_prepool(train_dataset, selected->model);
  const auto validation = selected->validation;
  const bool validation_strong = strong_gate(validation);
  const auto selected_index =
      static_cast<int64_t>(std::distance(candidates.begin(), selected));

  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::setprecision(17);
  output << "schema_id=" << kPrepoolSchema << '\n';
  output << "status=complete\n";
  output << "benchmark_id=synthetic_continuous_graph_v2\n";
  output << "diagnostic_phase=prepool_domain_scale\n";
  output << "diagnostic_authority=development_only\n";
  output << "benchmark_acceptance_authority=false\n";
  output << "phase2a_nine_head_authority_sha256=" << kPhase2AAuthoritySha
         << '\n';
  output << "frozen_parser_authority_sha256=" << kParserAuthoritySha << '\n';
  output << "probe_kind=" << kPrepoolSpec.name << '\n';
  output << "probe_record_schema=" << kPrepoolSpec.record_schema << '\n';
  output << "train_probe_rows=" << train_dataset.rows << '\n';
  output << "validation_probe_rows=" << validation_dataset.rows << '\n';
  output << "certified_probe_rows=0\n";
  output << "probe_rows_total=" << train_dataset.rows + validation_dataset.rows
         << '\n';
  output << "probe_ranges_disjoint=true\n";
  output << "fit_anchor_range=[0,2496)\n";
  output << "validation_anchor_range=[2560,2816)\n";
  output << "certified_anchor_range=not_opened\n";
  output << "purge_anchors_used=false\n";
  output << "maximum_anchor_read=2815\n";
  output << "final_holdout_begin=3328\n";
  output << "final_holdout_access=false\n";
  output << "policy_access=false\n";
  output << "refit_after_selection=false\n";
  output << "certified_candidates_scored=0\n";
  output << "feature_layout=base_256,quote_256,base_minus_quote_256\n";
  output << "probe_feature_width=" << kPrepoolSpec.probe_width << '\n';
  output << "affine_feature_width=" << kPrepoolSpec.affine_width << '\n';
  output << "edge_identity_feature_width_excluded=0\n";
  output << "summary_domain_count=2\n";
  output << "summary_scale_count=4\n";
  output << "summary_latent_width=32\n";
  output << "summary_group_count_per_channel=8\n";
  output << "summary_layout=domain_major_scale_minor_latent_minor\n";
  output << "fit_structure=one_weight_row_and_bias_per_edge_and_channel\n";
  output << "conditioned_head_count=" << kConditionedHeadCount << '\n';
  output << "standardization_scope=train_core_all_edges_all_channels\n";
  output << "solver=float64_centered_cholesky_ridge\n";
  output << "ridge_scaling=gram_diagonal_plus_edge_channel_sample_count_times_"
            "alpha\n";
  output << "ridge_grid=1e-12,1e-10,1e-8,1e-6,1e-4,1e-2\n";
  output << "selection_scope=one_global_candidate_for_all_edge_channel_heads\n";
  output << "selection_order=validation_direction,validation_rank,"
            "validation_correlation,validation_rmse,smallest_alpha\n";
  output << "selection_tie_tolerance=" << kTieTolerance << '\n';
  output << "cached_ridge_invariant_systems=true\n";
  output << "ridge_invariant_system_build_count="
         << solver_counters.ridge_invariant_system_build_count << '\n';
  output << "ridge_fit_attempt_count="
         << solver_counters.ridge_fit_attempt_count << '\n';
  output << "cholesky_factorization_count="
         << solver_counters.cholesky_factorization_count << '\n';
  output << "cholesky_solve_count=" << solver_counters.cholesky_solve_count
         << '\n';
  output << "maximum_cholesky_factorization_count=" << kMaximumCholeskyCount
         << '\n';
  output << "maximum_cholesky_solve_count=" << kMaximumCholeskyCount << '\n';
  output << "exclusive_output_creation=true\n";
  output << "numerically_valid_candidate_count="
         << std::count_if(candidates.begin(), candidates.end(),
                          [](const PrepoolCandidate &candidate) {
                            return candidate.numerically_valid;
                          })
         << '\n';
  output << "context_identity_max_abs_delta="
         << std::max(train_dataset.context_identity_max_abs_delta,
                     validation_dataset.context_identity_max_abs_delta)
         << '\n';

  for (std::size_t index = 0; index < candidates.size(); ++index) {
    const auto prefix = "candidate." + std::to_string(index);
    output << prefix << ".ridge=" << candidates[index].declared_ridge << '\n';
    output << prefix << ".numerically_valid="
           << (candidates[index].numerically_valid ? "true" : "false") << '\n';
    if (candidates[index].numerically_valid) {
      output << prefix << ".rejection_reason=\n";
      output << prefix << ".maximum_normalized_residual="
             << candidates[index].model.maximum_normalized_residual << '\n';
      output << prefix << ".coefficient_l2_norm="
             << candidates[index].model.coefficient_l2_norm << '\n';
      emit_metric(output, prefix + ".validation", candidates[index].validation);
    } else {
      output << prefix
             << ".rejection_reason=" << candidates[index].rejection_reason
             << '\n';
    }
  }

  output << "selected_candidate_index=" << selected_index << '\n';
  output << "selected_ridge=" << selected->model.ridge << '\n';
  output << "selected_maximum_normalized_residual="
         << selected->model.maximum_normalized_residual << '\n';
  output << "selected_coefficient_l2_norm="
         << selected->model.coefficient_l2_norm << '\n';
  emit_metric(output, "selected.train", train);
  emit_metric(output, "selected.validation", validation);
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    emit_metric(
        output, "selected.train.channel_" + std::to_string(channel),
        evaluate_prepool_channel(train_dataset, selected->model, channel));
    emit_metric(
        output, "selected.validation.channel_" + std::to_string(channel),
        evaluate_prepool_channel(validation_dataset, selected->model, channel));
  }
  output << "validation_strong_gate_pass="
         << (validation_strong ? "true" : "false") << '\n';
  output << "certified_strong_gate_pass=not_evaluated\n";
  output << "validation_partial_gate_pass="
         << (partial_gate(validation) ? "true" : "false") << '\n';
  output << "certified_partial_gate_pass=not_evaluated\n";
  output << "rung_b_authorized=false\n";
  output << "classification="
         << (validation_strong
                 ? "prepool_domain_scale_affine_strong_gate_observed_"
                   "development_only"
                 : "prepool_domain_scale_affine_strong_gate_not_observed")
         << '\n';
  output << "preregistered_strong_gate=direction>=0.95,rank>=0.95,"
            "correlation>=0.95,rmse_target_rms_ratio<=0.25\n";
  output << "preregistered_partial_gate=direction>=0.80,rank>=0.78\n";
  if (!output) {
    throw std::runtime_error("failed while rendering output report");
  }
  write_exclusive_report(options.output, output.str());
}

} // namespace

int main(int argc, char **argv) {
  try {
    std::locale::global(std::locale::classic());
    run_prepool(parse_prepool_options(argc, argv));
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "frozen MTF pre-pool domain-scale affine probe: "
              << error.what() << '\n';
    return 1;
  }
}
