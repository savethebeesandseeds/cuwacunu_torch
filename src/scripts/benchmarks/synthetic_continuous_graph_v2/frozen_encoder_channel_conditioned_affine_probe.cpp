// Development-only Phase 2A channel-conditioned affine evaluator.
//
// Keep the canonical probe parser, metric implementation, ridge grid, and
// validation comparator byte-for-byte shared with the frozen affine evaluator.
// Its main entry point is renamed so this translation unit can provide the
// narrower channel-conditioned development contract below.
#define main cuwacunu_embedded_pooled_affine_probe_main
#include "frozen_representation_affine_probe.cpp"
#undef main

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

namespace {

constexpr std::string_view kChannelConditionedSchema =
    "synthetic_v2_frozen_encoder_channel_conditioned_affine_development_v1";

struct ChannelConditionedModel {
  torch::Tensor mean;    // [D], train core across all edges and channels.
  torch::Tensor inv_std; // [D], train core across all edges and channels.
  torch::Tensor weights; // [E,C,D], one independent row per edge x channel.
  torch::Tensor bias;    // [E,C], one independent bias per edge x channel.
  int64_t feature_width{0};
  double ridge{0.0};
  double maximum_normalized_residual{0.0};
  double coefficient_l2_norm{0.0};
};

struct ChannelConditionedCandidate {
  bool numerically_valid{false};
  double declared_ridge{0.0};
  std::string rejection_reason;
  ChannelConditionedModel model;
  MetricSummary validation;
};

void write_exclusive_report(const std::filesystem::path &path,
                            const std::string &contents) {
  const int descriptor = ::open(path.c_str(),
                                O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
  if (descriptor < 0) {
    throw std::runtime_error(
        "output report path must be absent and exclusively creatable: " +
        path.string() + ": " + std::strerror(errno));
  }

  int open_descriptor = descriptor;
  try {
    std::size_t offset = 0;
    while (offset < contents.size()) {
      const auto written =
          ::write(open_descriptor, contents.data() + offset,
                  contents.size() - offset);
      if (written < 0 && errno == EINTR) {
        continue;
      }
      if (written <= 0) {
        throw std::runtime_error("failed while writing exclusive output report: " +
                                 std::string(std::strerror(errno)));
      }
      offset += static_cast<std::size_t>(written);
    }
    if (::close(open_descriptor) != 0) {
      open_descriptor = -1;
      throw std::runtime_error("failed while closing exclusive output report: " +
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

torch::Tensor
channel_conditioned_predict(const ChannelConditionedModel &model,
                            const torch::Tensor &features) {
  torch::NoGradGuard no_grad;
  const auto feature_width = model.feature_width;
  if (feature_width <= 0 || features.dim() != 4 ||
      features.size(1) != kEdgeCount ||
      features.size(2) != kChannelCount ||
      features.size(3) != feature_width) {
    throw std::runtime_error(
        "channel-conditioned model and feature shapes differ");
  }
  const auto standardized =
      (features - model.mean.view({1, 1, 1, feature_width})) *
      model.inv_std.view({1, 1, 1, feature_width});
  return (standardized *
          model.weights.view(
              {1, kEdgeCount, kChannelCount, feature_width}))
             .sum(-1) +
         model.bias.view({1, kEdgeCount, kChannelCount});
}

ChannelConditionedModel channel_conditioned_fit(const Dataset &dataset,
                                                double ridge) {
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
  if (features.dim() != 4 || target.dim() != 3 ||
      features.size(0) != kTrainEnd - kTrainBegin ||
      features.size(1) != kEdgeCount ||
      features.size(2) != kChannelCount ||
      target.sizes() !=
          torch::IntArrayRef(
              {kTrainEnd - kTrainBegin, kEdgeCount, kChannelCount})) {
    throw std::runtime_error(
        "channel-conditioned fit tensor contract mismatch");
  }
  const auto feature_width = features.size(3);
  if (feature_width != kRepresentationSpec.affine_width) {
    throw std::runtime_error(
        "channel-conditioned fit requires the canonical raw96 feature width");
  }

  // Preserve the canonical affine evaluator's standardization scope. Only the
  // fitted coefficient structure changes in Phase 2A.
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

  auto weights = torch::zeros(
      {kEdgeCount, kChannelCount, feature_width}, torch::kFloat64);
  auto bias =
      torch::zeros({kEdgeCount, kChannelCount}, torch::kFloat64);
  double maximum_residual = 0.0;
  for (int64_t edge = 0; edge < kEdgeCount; ++edge) {
    for (int64_t channel = 0; channel < kChannelCount; ++channel) {
      const auto x = standardized.select(1, edge)
                         .select(1, channel)
                         .contiguous();
      const auto y =
          target.select(1, edge).select(1, channel).contiguous();
      if (x.sizes() !=
              torch::IntArrayRef(
                  {kTrainEnd - kTrainBegin, feature_width}) ||
          y.sizes() !=
              torch::IntArrayRef({kTrainEnd - kTrainBegin})) {
        throw std::runtime_error(
            "edge-channel fitting slice has an unexpected shape");
      }
      const auto x_mean = x.mean(0);
      const auto y_mean = y.mean();
      const auto centered_x = x - x_mean;
      const auto centered_y = y - y_mean;
      auto gram = centered_x.transpose(0, 1).matmul(centered_x);
      gram.diagonal(0, 0, 1).add_(static_cast<double>(x.size(0)) * ridge);
      const auto rhs =
          centered_x.transpose(0, 1).matmul(centered_y.unsqueeze(1));
      auto [cholesky, info] = at::linalg_cholesky_ex(gram, false, false);
      if (info.item<int64_t>() != 0) {
        throw CandidateNumericalError("cholesky_factorization_failed");
      }
      const auto row =
          at::cholesky_solve(rhs, cholesky, false).squeeze(1);
      const auto residual = gram.matmul(row.unsqueeze(1)) - rhs;
      const double normalized_residual =
          residual.norm().item<double>() /
          std::max(rhs.norm().item<double>(), 1.0e-30);
      if (!std::isfinite(normalized_residual) ||
          normalized_residual > 1.0e-7 ||
          !torch::isfinite(row).all().item<bool>()) {
        throw CandidateNumericalError(
            "normalized_residual_or_finiteness_failed");
      }
      maximum_residual =
          std::max(maximum_residual, normalized_residual);
      weights.select(0, edge).select(0, channel).copy_(row);
      bias.select(0, edge)
          .select(0, channel)
          .copy_(y_mean - x_mean.dot(row));
    }
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

MetricSummary channel_conditioned_evaluate(
    const Dataset &dataset, const ChannelConditionedModel &model) {
  return summarize(observe(
      channel_conditioned_predict(model, dataset.features), dataset.target));
}

MetricSummary channel_conditioned_evaluate_channel(
    const Dataset &dataset, const ChannelConditionedModel &model,
    int64_t channel) {
  if (channel < 0 || channel >= kChannelCount) {
    throw std::runtime_error("invalid channel index");
  }
  return summarize(observe(
      channel_conditioned_predict(model, dataset.features)
          .narrow(2, channel, 1),
      dataset.target.narrow(2, channel, 1)));
}

// Reuse the canonical global validation comparator without copying or
// changing its priority order. Only the ridge and validation fields are read
// by better().
Candidate comparator_view(const ChannelConditionedCandidate &candidate) {
  Candidate view;
  view.numerically_valid = candidate.numerically_valid;
  view.declared_ridge = candidate.declared_ridge;
  view.rejection_reason = candidate.rejection_reason;
  view.model.ridge = candidate.model.ridge;
  view.validation = candidate.validation;
  return view;
}

bool channel_conditioned_better(
    const ChannelConditionedCandidate &candidate,
    const ChannelConditionedCandidate &incumbent) {
  return better(comparator_view(candidate), comparator_view(incumbent));
}

void run_channel_conditioned(const Options &options) {
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  torch::manual_seed(31);

  if (options.probe_spec != &kRepresentationSpec) {
    throw std::runtime_error(
        "Phase 2A accepts only --probe-kind representation");
  }
  if (!options.development_only || options.validation_only ||
      !options.certified_input.empty() || !options.selection_lock.empty()) {
    throw std::runtime_error(
        "Phase 2A requires --development-only and has no certified, final, "
        "selection-lock, or validation-control mode");
  }

  const auto train_dataset = read_probe(
      options.train_input, kTrainBegin, kTrainEnd, kRepresentationSpec);
  const auto validation_dataset =
      read_probe(options.validation_input, kValidationBegin, kValidationEnd,
                 kRepresentationSpec);

  std::vector<ChannelConditionedCandidate> candidates;
  candidates.reserve(kRidgeGrid.size());
  for (const double ridge : kRidgeGrid) {
    try {
      auto model = channel_conditioned_fit(train_dataset, ridge);
      auto validation =
          channel_conditioned_evaluate(validation_dataset, model);
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
    if (selected == candidates.end() ||
        channel_conditioned_better(*candidate, *selected)) {
      selected = candidate;
    }
  }
  if (selected == candidates.end()) {
    throw std::runtime_error("no ridge candidate was selected");
  }

  const auto train =
      channel_conditioned_evaluate(train_dataset, selected->model);
  const auto validation = selected->validation;
  const bool validation_strong = strong_gate(validation);
  const auto selected_index =
      static_cast<int64_t>(std::distance(candidates.begin(), selected));

  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::setprecision(17);
  output << "schema_id=" << kChannelConditionedSchema << '\n';
  output << "status=complete\n";
  output << "benchmark_id=synthetic_continuous_graph_v2\n";
  output << "diagnostic_phase=2A\n";
  output << "diagnostic_authority=development_only\n";
  output << "benchmark_acceptance_authority=false\n";
  output << "probe_kind=representation\n";
  output << "probe_record_schema=" << kRepresentationSpec.record_schema
         << '\n';
  output << "train_probe_rows=" << train_dataset.rows << '\n';
  output << "validation_probe_rows=" << validation_dataset.rows << '\n';
  output << "certified_probe_rows=0\n";
  output << "probe_rows_total="
         << train_dataset.rows + validation_dataset.rows << '\n';
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
  output << "feature_layout=base_32,quote_32,base_minus_quote_32\n";
  output << "probe_feature_width=96\n";
  output << "affine_feature_width=96\n";
  output << "edge_identity_feature_width_excluded=0\n";
  output << "fit_structure=one_weight_row_and_bias_per_edge_and_channel\n";
  output << "conditioned_head_count=9\n";
  output << "standardization_scope=train_core_all_edges_all_channels\n";
  output << "solver=float64_centered_cholesky_ridge\n";
  output << "ridge_scaling=gram_diagonal_plus_edge_channel_sample_count_times_"
            "alpha\n";
  output << "ridge_grid=1e-12,1e-10,1e-8,1e-6,1e-4,1e-2\n";
  output << "selection_scope=one_global_candidate_for_all_edge_channel_heads\n";
  output << "selection_order=validation_direction,validation_rank,"
            "validation_correlation,validation_rmse,smallest_alpha\n";
  output << "selection_tie_tolerance=" << kTieTolerance << '\n';
  output << "numerically_valid_candidate_count="
         << std::count_if(
                candidates.begin(), candidates.end(),
                [](const ChannelConditionedCandidate &candidate) {
                  return candidate.numerically_valid;
                })
         << '\n';
  output << "context_identity_max_abs_delta="
         << std::max(train_dataset.context_identity_max_abs_delta,
                     validation_dataset.context_identity_max_abs_delta)
         << '\n';

  for (std::size_t index = 0; index < candidates.size(); ++index) {
    const auto prefix = "candidate." + std::to_string(index);
    output << prefix << ".ridge=" << candidates[index].declared_ridge
           << '\n';
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

  output << "selected_candidate_index=" << selected_index << '\n';
  output << "selected_ridge=" << selected->model.ridge << '\n';
  output << "selected_maximum_normalized_residual="
         << selected->model.maximum_normalized_residual << '\n';
  output << "selected_coefficient_l2_norm="
         << selected->model.coefficient_l2_norm << '\n';
  emit_metric(output, "selected.train", train);
  emit_metric(output, "selected.validation", validation);
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    emit_metric(output, "selected.train.channel_" + std::to_string(channel),
                channel_conditioned_evaluate_channel(
                    train_dataset, selected->model, channel));
    emit_metric(output,
                "selected.validation.channel_" + std::to_string(channel),
                channel_conditioned_evaluate_channel(
                    validation_dataset, selected->model, channel));
  }
  output << "validation_strong_gate_pass="
         << (validation_strong ? "true" : "false") << '\n';
  output << "certified_strong_gate_pass=not_evaluated\n";
  output << "validation_partial_gate_pass="
         << (partial_gate(validation) ? "true" : "false") << '\n';
  output << "certified_partial_gate_pass=not_evaluated\n";
  output << "rung_b_authorized="
         << (validation_strong ? "false" : "true") << '\n';
  output << "classification="
         << (validation_strong
                 ? "edge_channel_affine_sufficiency_established"
                 : "edge_channel_affine_sufficiency_not_established")
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
    run_channel_conditioned(parse_options(argc, argv));
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "frozen encoder channel-conditioned affine probe: "
              << error.what() << '\n';
    return 1;
  }
}
