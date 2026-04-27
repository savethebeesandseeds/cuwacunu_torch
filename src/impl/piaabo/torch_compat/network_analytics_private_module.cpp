void append_topk_pretty_lines_(
    std::ostringstream* oss,
    std::string_view label,
    const std::vector<analytics_topk_entry_t>& entries,
    const char* c_key,
    const char* c_value,
    const char* c_note,
    const char* c_reset) {
  if (oss == nullptr) return;
  if (entries.empty()) {
    *oss << "\t" << c_key << std::left << std::setw(34) << label << c_reset
         << " : " << c_note << "(none)" << c_reset << "\n";
    return;
  }
  for (std::size_t i = 0; i < entries.size(); ++i) {
    std::ostringstream key;
    key << label << "[" << (i + 1) << "]";
    std::ostringstream value;
    value.setf(std::ios::fixed);
    value << std::setprecision(6) << entries[i].value;
    *oss << "\t" << c_key << std::left << std::setw(34) << key.str() << c_reset
         << " : " << c_value << entries[i].tensor_name << c_reset
         << "  " << c_note << value.str() << c_reset << "\n";
  }
}

[[nodiscard]] std::string as_pretty_text_(
    const network_analytics_report_t& report,
    std::string_view network_label,
    bool use_color) {
  const char* c_reset = use_color ? "\x1b[0m" : "";
  const char* c_title = use_color ? "\x1b[1;96m" : "";
  const char* c_section = use_color ? "\x1b[1;94m" : "";
  const char* c_key = use_color ? "\x1b[90m" : "";
  const char* c_value = use_color ? "\x1b[97m" : "";
  const char* c_note = use_color ? "\x1b[36m" : "";
  const char* c_good = use_color ? "\x1b[92m" : "";
  const char* c_bad = use_color ? "\x1b[91m" : "";
  const char* c_warn = use_color ? "\x1b[93m" : "";
  const char* c_entropy = use_color ? "\x1b[1;95m" : "";

  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(6);

  auto line = [&](std::string_view key,
                  const auto& value,
                  std::string_view note) {
    oss << "\t" << c_key << std::left << std::setw(34) << key << c_reset
        << " : " << c_value << value << c_reset << "\t" << c_note << note
        << c_reset << "\n";
  };
  auto line_color = [&](std::string_view key,
                        const auto& value,
                        const char* color,
                        std::string_view note) {
    oss << "\t" << c_key << std::left << std::setw(34) << key << c_reset
        << " : " << color << value << c_reset << "\t" << c_note << note
        << c_reset << "\n";
  };
  auto section = [&](std::string_view title) {
    oss << c_section << title << c_reset << "\n";
  };

  oss << c_title << "Network Analytics Report" << c_reset << "\n";
  line("schema", report.schema, "report schema id");
  if (!network_label.empty()) {
    line("network_label", network_label, "network/component under analysis");
  }
  line("tensor_count", report.tensor_count, "parameter tensors visited");
  line("trainable_tensor_count",
       report.trainable_tensor_count,
       "requires_grad=true tensors");
  line("anomaly_top_k",
       report.normalized_options.anomaly_top_k,
       "top-k entries retained per anomaly family");
  oss << "\n";

  section("Health Snapshot");
  line_color("finite_ratio",
             report.finite_ratio,
             (report.finite_ratio >= 0.999999) ? c_good : c_warn,
             "finite / total");
  line_color("non_finite_ratio",
             report.non_finite_ratio,
             (report.non_finite_ratio <= 0.0) ? c_good : c_bad,
             "(nan + inf) / total");
  line("near_zero_ratio", report.near_zero_ratio, "fraction near zero threshold");
  line("stddev", report.stddev, "global finite stddev");
  line("max_abs", report.max_abs, "global finite absolute maximum");
  line("max_abs_over_rms", report.max_abs_over_rms, "outlier heaviness");
  line("tensor_rms_cv", report.tensor_rms_cv, "cross-tensor RMS variability");
  line("abs_energy_entropy",
       report.abs_energy_entropy,
       "normalized abs-energy entropy");
  oss << "\n";

  section("Network Entropy Capacity");
  line_color("Network Entropy Capacity",
             report.network_global_entropic_capacity,
             c_entropy,
             "primary capacity indicator");
  line("network_entropic_bottleneck_min",
       report.network_entropic_bottleneck_min,
       "lowest effective-rank bottleneck");
  line("network_effective_rank_p50",
       report.network_effective_rank_p50,
       "effective-rank median");
  line("network_effective_rank_p90",
       report.network_effective_rank_p90,
       "effective-rank p90");
  line("network_capacity_tensor_count",
       report.network_capacity_tensor_count,
       "tensors participating in capacity rollup");
  line("spectral_tensor_count",
       report.spectral_tensor_count,
       "tensors with spectral metrics");
  line_color("spectral_failed_tensor_count",
             report.spectral_failed_tensor_count,
             (report.spectral_failed_tensor_count == 0) ? c_good : c_bad,
             "SVD failures");
  oss << "\n";

  section("Top Alerts");
  append_topk_pretty_lines_(
      &oss,
      "top_nonfinite_ratio",
      report.top_nonfinite_ratio_tensors,
      c_key,
      c_value,
      c_note,
      c_reset);
  append_topk_pretty_lines_(
      &oss,
      "top_max_abs_over_rms",
      report.top_max_abs_over_rms_tensors,
      c_key,
      c_value,
      c_note,
      c_reset);
  append_topk_pretty_lines_(
      &oss,
      "top_low_effective_rank",
      report.top_low_effective_rank_tensors,
      c_key,
      c_value,
      c_note,
      c_reset);

  return oss.str();
}

[[nodiscard]] std::string as_ascii_key_value_(
    const network_design_analytics_report_t& report,
    std::string_view source_label) {
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(10);

  oss << "schema=" << report.schema << "\n";
  if (!source_label.empty()) {
    oss << "source_label=" << source_label << "\n";
  }
  oss << "analysis_valid=" << bool_to_ascii(report.analysis_valid) << "\n";
  if (!report.analysis_error.empty()) {
    oss << "analysis_error=" << report.analysis_error << "\n";
  }
  oss << "duplicate_node_id_count=" << report.duplicate_node_id_count << "\n";
  if (!report.duplicate_node_id_example.empty()) {
    oss << "duplicate_node_id_example=" << report.duplicate_node_id_example << "\n";
  }
  oss << "network_id=" << report.network_id << "\n";
  oss << "assembly_tag=" << report.assembly_tag << "\n";
  oss << "node_count=" << report.node_count << "\n";
  oss << "export_count=" << report.export_count << "\n";
  oss << "parameter_count=" << report.parameter_count << "\n";
  oss << "internal_edge_count=" << report.internal_edge_count << "\n";
  oss << "export_edge_count=" << report.export_edge_count << "\n";
  oss << "total_edge_count=" << report.total_edge_count << "\n";
  oss << "weakly_connected_components=" << report.weakly_connected_components
      << "\n";
  oss << "isolated_node_count=" << report.isolated_node_count << "\n";
  oss << "has_internal_cycle=" << bool_to_ascii(report.has_internal_cycle) << "\n";
  oss << "acyclic_longest_path_nodes=" << report.acyclic_longest_path_nodes
      << "\n";
  oss << "internal_edge_density=" << report.internal_edge_density << "\n";
  oss << "mean_in_degree=" << report.mean_in_degree << "\n";
  oss << "mean_out_degree=" << report.mean_out_degree << "\n";
  oss << "max_in_degree=" << report.max_in_degree << "\n";
  oss << "max_out_degree=" << report.max_out_degree << "\n";
  oss << "distinct_node_kind_count=" << report.distinct_node_kind_count << "\n";
  oss << "node_kind_entropy=" << report.node_kind_entropy << "\n";
  oss << "in_degree_entropy=" << report.in_degree_entropy << "\n";
  oss << "out_degree_entropy=" << report.out_degree_entropy << "\n";
  oss << "edge_kind_transition_entropy=" << report.edge_kind_transition_entropy
      << "\n";

  oss << "topological_order_count=" << report.topological_order_count << "\n";
  oss << "topological_order_ratio=" << report.topological_order_ratio << "\n";
  oss << "edge_surplus=" << report.edge_surplus << "\n";
  oss << "active_fanout_mean=" << report.active_fanout_mean << "\n";

  oss << "source_count=" << report.source_count << "\n";
  oss << "internal_sink_count=" << report.internal_sink_count << "\n";
  oss << "export_reachable_node_count=" << report.export_reachable_node_count
      << "\n";
  oss << "export_reachable_ratio=" << report.export_reachable_ratio << "\n";
  oss << "dead_end_node_count=" << report.dead_end_node_count << "\n";
  oss << "orphan_node_count=" << report.orphan_node_count << "\n";
  oss << "longest_source_to_export_path_nodes="
      << report.longest_source_to_export_path_nodes << "\n";
  oss << "median_source_to_export_path_nodes="
      << report.median_source_to_export_path_nodes << "\n";
  oss << "branch_node_count=" << report.branch_node_count << "\n";
  oss << "merge_node_count=" << report.merge_node_count << "\n";

  oss << "scc_count=" << report.scc_count << "\n";
  oss << "largest_scc_size=" << report.largest_scc_size << "\n";
  oss << "cyclic_node_count=" << report.cyclic_node_count << "\n";
  oss << "cyclic_node_ratio=" << report.cyclic_node_ratio << "\n";

  oss << "skip_edge_count=" << report.skip_edge_count << "\n";
  oss << "skip_edge_ratio=" << report.skip_edge_ratio << "\n";
  oss << "mean_skip_span=" << report.mean_skip_span << "\n";
  oss << "max_skip_span=" << report.max_skip_span << "\n";

  oss << "explicit_edge_count=" << report.explicit_edge_count << "\n";
  oss << "inferred_edge_count=" << report.inferred_edge_count << "\n";
  oss << "unresolved_identifier_token_count="
      << report.unresolved_identifier_token_count << "\n";
  oss << "self_reference_token_count=" << report.self_reference_token_count
      << "\n";

  return oss.str();
}

[[nodiscard]] std::string as_pretty_text_(
    const network_design_analytics_report_t& report,
    std::string_view source_label,
    bool use_color) {
  const char* c_reset = use_color ? "\x1b[0m" : "";
  const char* c_title = use_color ? "\x1b[1;96m" : "";
  const char* c_section = use_color ? "\x1b[1;94m" : "";
  const char* c_key = use_color ? "\x1b[90m" : "";
  const char* c_value = use_color ? "\x1b[97m" : "";
  const char* c_note = use_color ? "\x1b[36m" : "";
  const char* c_good = use_color ? "\x1b[92m" : "";
  const char* c_bad = use_color ? "\x1b[91m" : "";

  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(10);

  auto line = [&](std::string_view key,
                  const auto& value,
                  std::string_view note) {
    oss << "\t" << c_key << std::left << std::setw(34) << key << c_reset << " : "
        << c_value << value << c_reset << "\t" << c_note << note << c_reset
        << "\n";
  };
  auto line_color = [&](std::string_view key,
                        const auto& value,
                        const char* color,
                        std::string_view note) {
    oss << "\t" << c_key << std::left << std::setw(34) << key << c_reset << " : "
        << color << value << c_reset << "\t" << c_note << note << c_reset << "\n";
  };
  auto section = [&](std::string_view name) {
    oss << c_section << name << c_reset << "\n";
  };

  oss << c_title << "Network Design Analytics Report" << c_reset << "\n";
  if (!source_label.empty()) {
    line("source_label", source_label, "contract/key/path reference");
  }
  oss << "\t" << c_key << std::left << std::setw(34) << "metric" << c_reset
      << " : " << c_value << "value" << c_reset << "\t" << c_note
      << "comment + best guidance" << c_reset << "\n\n";

  section("Identity");
  line("schema", report.schema, "report schema id (best: fixed)");
  line_color("analysis_valid",
             bool_to_ascii(report.analysis_valid),
             report.analysis_valid ? c_good : c_bad,
             "input validity state");
  if (!report.analysis_error.empty()) {
    line("analysis_error", report.analysis_error, "invalid-report reason");
  }
  if (report.duplicate_node_id_count > 0) {
    line("duplicate_node_id_count",
         report.duplicate_node_id_count,
         "extra node ids rejected before graph analysis");
  }
  if (!report.duplicate_node_id_example.empty()) {
    line("duplicate_node_id_example",
         report.duplicate_node_id_example,
         "first duplicate node id encountered");
  }
  line("network_id", report.network_id, "canonical network id (best: stable)");
  line("assembly_tag", report.assembly_tag, "assembly tag (best: expected)");
  oss << "\n";

  section("Graph Size");
  line("node_count", report.node_count, "declared node blocks (best: task-dependent)");
  line("export_count", report.export_count, "declared exports (best: >=1)");
  line("parameter_count", report.parameter_count, "node params declared (best: minimal-complete)");
  line("internal_edge_count", report.internal_edge_count, "node-to-node edges (best: task-dependent)");
  line("export_edge_count", report.export_edge_count, "node-to-export edges (best: >=1)");
  line("total_edge_count", report.total_edge_count, "internal + export edges");
  oss << "\n";

  section("Topology");
  line("weakly_connected_components",
       report.weakly_connected_components,
       "disconnected groups (best: close to 1)");
  line("isolated_node_count",
       report.isolated_node_count,
       "nodes with no edges (best: 0)");
  line_color("has_internal_cycle",
             bool_to_ascii(report.has_internal_cycle),
             report.has_internal_cycle ? c_bad : c_good,
             "cycle in internal graph (best: false for feed-forward)");
  line("topological_order_count",
       report.topological_order_count,
       "sortable nodes count");
  line("topological_order_ratio",
       report.topological_order_ratio,
       "sortable coverage ratio in [0,1]");
  line("acyclic_longest_path_nodes",
       report.acyclic_longest_path_nodes,
       "DAG-only depth proxy");
  oss << "\n";

  section("Complexity");
  line("internal_edge_density",
       report.internal_edge_density,
       "normalized connectivity [0..1] (best: task-dependent)");
  line("edge_surplus",
       report.edge_surplus,
       "E-N+C edge surplus proxy");
  line("mean_in_degree",
       report.mean_in_degree,
       "avg incoming edges per node (best: balanced)");
  line("mean_out_degree",
       report.mean_out_degree,
       "avg outgoing edges per node (best: balanced)");
  line("max_in_degree",
       report.max_in_degree,
       "largest fan-in (best: avoid extreme bottlenecks)");
  line("max_out_degree",
       report.max_out_degree,
       "largest fan-out (best: avoid extreme branching)");
  line("active_fanout_mean",
       report.active_fanout_mean,
       "avg out-degree among nodes with out-degree>0");
  oss << "\n";

  section("Directionality");
  line("source_count", report.source_count, "nodes with in-degree 0");
  line("internal_sink_count", report.internal_sink_count, "nodes with internal out-degree 0");
  line("export_reachable_node_count",
       report.export_reachable_node_count,
       "nodes that can reach any export");
  line("export_reachable_ratio",
       report.export_reachable_ratio,
       "reachable/export-connected node ratio");
  line("dead_end_node_count", report.dead_end_node_count, "nodes reaching no export");
  line("orphan_node_count", report.orphan_node_count, "nodes unreachable from sources");
  line("longest_source_to_export_path_nodes",
       report.longest_source_to_export_path_nodes,
       "condensation-DAG longest path (node-count weighted)");
  line("median_source_to_export_path_nodes",
       report.median_source_to_export_path_nodes,
       "median export-path length");
  line("branch_node_count", report.branch_node_count, "nodes with internal out-degree > 1");
  line("merge_node_count", report.merge_node_count, "nodes with internal in-degree > 1");
  oss << "\n";

  section("Cycle Burden");
  line("scc_count", report.scc_count, "strongly connected component count");
  line("largest_scc_size", report.largest_scc_size, "largest SCC cardinality");
  line("cyclic_node_count", report.cyclic_node_count, "nodes in cyclic SCCs");
  line("cyclic_node_ratio", report.cyclic_node_ratio, "cyclic node fraction");
  oss << "\n";

  section("Skip Geometry");
  line("skip_edge_count", report.skip_edge_count, "cross-SCC edges with span > 1");
  line("skip_edge_ratio", report.skip_edge_ratio, "skip edges / internal edges");
  line("mean_skip_span", report.mean_skip_span, "mean span over skip edges");
  line("max_skip_span", report.max_skip_span, "max skip span");
  oss << "\n";

  section("Edge Evidence");
  line("explicit_edge_count", report.explicit_edge_count, "explicit references (phase-1 fixed at 0)");
  line("inferred_edge_count", report.inferred_edge_count, "token-inferred internal edges");
  line("unresolved_identifier_token_count",
       report.unresolved_identifier_token_count,
       "tokens not resolved to node ids");
  line("self_reference_token_count",
       report.self_reference_token_count,
       "tokens resolving to their own node id");
  oss << "\n";

  section("Entropy and Diversity");
  line("distinct_node_kind_count",
       report.distinct_node_kind_count,
       "unique node kinds (best: task-dependent)");
  line("node_kind_entropy",
       report.node_kind_entropy,
       "balance across node kinds (best: task-dependent)");
  line("in_degree_entropy",
       report.in_degree_entropy,
       "diversity of in-degree distribution (best: task-dependent)");
  line("out_degree_entropy",
       report.out_degree_entropy,
       "diversity of out-degree distribution (best: task-dependent)");
  line("edge_kind_transition_entropy",
       report.edge_kind_transition_entropy,
       "diversity of kind->kind transitions (best: task-dependent)");

  return oss.str();
}

}  // namespace

network_analytics_report_t summarize_module_network_analytics(
    const torch::nn::Module& model,
    const network_analytics_options_t& options) {
  const network_analytics_options_t effective = normalize_options_(options);

  network_analytics_report_t out{};
  out.normalized_options = effective;
  out.near_zero_threshold = effective.near_zero_epsilon;

  const std::int64_t bins =
      std::max<std::int64_t>(1, effective.log10_abs_histogram_bins);
  std::vector<std::uint64_t> hist_counts(static_cast<std::size_t>(bins), 0ULL);
  std::vector<double> exact_logabs_samples{};
  if (bins == 1) {
    exact_logabs_samples.reserve(1024);
  }

  report_accumulator_t acc{};
  buffer_accumulator_t buffer_acc{};
  spectral_aggregate_t spectral_acc{};

  std::vector<analytics_topk_entry_t> nonfinite_candidates{};
  std::vector<analytics_topk_entry_t> max_abs_over_rms_candidates{};
  std::vector<analytics_topk_entry_t> near_zero_candidates{};
  std::vector<analytics_topk_entry_t> low_stable_rank_candidates{};
  std::vector<analytics_topk_entry_t> low_effective_rank_candidates{};
  std::vector<analytics_topk_entry_t> high_spectral_norm_candidates{};

  for (const auto& named_param : model.named_parameters(/*recurse=*/true)) {
    const std::string tensor_name = named_param.key();
    const auto& param = named_param.value();
    const tensor_snapshot_t snapshot = analyze_parameter_tensor_(
        tensor_name,
        param,
        effective,
        &hist_counts,
        (bins == 1) ? &exact_logabs_samples : nullptr,
        &acc,
        &spectral_acc);

    if (snapshot.total_count > 0) {
      nonfinite_candidates.push_back(
          analytics_topk_entry_t{snapshot.tensor_name, snapshot.non_finite_ratio});
    }
    if (snapshot.finite_count > 0) {
      max_abs_over_rms_candidates.push_back(
          analytics_topk_entry_t{snapshot.tensor_name, snapshot.max_abs_over_rms});
      near_zero_candidates.push_back(
          analytics_topk_entry_t{snapshot.tensor_name, snapshot.near_zero_ratio});
    }
    if (snapshot.spectral_computed) {
      low_stable_rank_candidates.push_back(
          analytics_topk_entry_t{snapshot.tensor_name, snapshot.stable_rank});
      low_effective_rank_candidates.push_back(
          analytics_topk_entry_t{snapshot.tensor_name, snapshot.effective_rank});
      high_spectral_norm_candidates.push_back(
          analytics_topk_entry_t{snapshot.tensor_name, snapshot.spectral_norm});
    }
  }

  if (effective.include_buffers) {
    for (const auto& named_buffer : model.named_buffers(/*recurse=*/true)) {
      accumulate_buffer_tensor_(
          named_buffer.key(),
          named_buffer.value(),
          &buffer_acc);
    }
  }

  out.tensor_count = acc.tensor_count;
  out.trainable_tensor_count = acc.trainable_tensor_count;
  out.total_parameter_count = acc.total_parameter_count;
  out.finite_parameter_count = acc.finite_parameter_count;
  out.nan_parameter_count = acc.nan_parameter_count;
  out.inf_parameter_count = acc.inf_parameter_count;

  const double total = static_cast<double>(out.total_parameter_count);
  const double finite = static_cast<double>(out.finite_parameter_count);
  out.finite_ratio = safe_ratio(finite, total);
  out.non_finite_ratio = safe_ratio(
      static_cast<double>(out.nan_parameter_count + out.inf_parameter_count),
      total);

  if (out.finite_parameter_count > 0) {
    const long double finite_ld = static_cast<long double>(out.finite_parameter_count);
    const double mean = static_cast<double>(acc.sum / finite_ld);
    const double ex2 = static_cast<double>(acc.sum_sq / finite_ld);
    out.stddev = std::sqrt(std::max(0.0, ex2 - mean * mean));
    out.min = std::isfinite(acc.min) ? acc.min : 0.0;
    out.max = std::isfinite(acc.max) ? acc.max : 0.0;

    out.l1_mean_abs = static_cast<double>(acc.sum_abs / finite_ld);
    out.l2_rms = std::sqrt(std::max(0.0, static_cast<double>(acc.sum_sq / finite_ld)));
    out.max_abs = acc.max_abs;
    out.max_abs_over_rms = safe_ratio(out.max_abs, out.l2_rms + kNumericEpsilon);

    out.near_zero_ratio =
        safe_ratio(static_cast<double>(acc.near_zero_count), finite);
    out.exact_zero_ratio =
        safe_ratio(static_cast<double>(acc.exact_zero_count), finite);

    out.abs_energy_entropy = normalized_abs_energy_entropy_(
        acc.non_zero_abs_count,
        acc.sum_abs,
        acc.sum_abs_log_abs);
    out.log10_abs_histogram_entropy =
        (bins == 1) ? 0.0
                    : normalized_histogram_entropy_(
                          hist_counts, out.finite_parameter_count);

    double p25 = 0.0;
    double p50 = 0.0;
    double p75 = 0.0;
    double p90 = 0.0;
    double p99 = 0.0;
    if (bins == 1 && !exact_logabs_samples.empty()) {
      p25 = sorted_quantile_(exact_logabs_samples, 0.25);
      p50 = sorted_quantile_(exact_logabs_samples, 0.50);
      p75 = sorted_quantile_(exact_logabs_samples, 0.75);
      p90 = sorted_quantile_(exact_logabs_samples, 0.90);
      p99 = sorted_quantile_(exact_logabs_samples, 0.99);
    } else {
      p25 = histogram_quantile_log10_(
          hist_counts,
          effective.log10_abs_histogram_min,
          effective.log10_abs_histogram_max,
          0.25);
      p50 = histogram_quantile_log10_(
          hist_counts,
          effective.log10_abs_histogram_min,
          effective.log10_abs_histogram_max,
          0.50);
      p75 = histogram_quantile_log10_(
          hist_counts,
          effective.log10_abs_histogram_min,
          effective.log10_abs_histogram_max,
          0.75);
      p90 = histogram_quantile_log10_(
          hist_counts,
          effective.log10_abs_histogram_min,
          effective.log10_abs_histogram_max,
          0.90);
      p99 = histogram_quantile_log10_(
          hist_counts,
          effective.log10_abs_histogram_min,
          effective.log10_abs_histogram_max,
          0.99);
    }

    out.log10_abs_p50 = p50;
    out.log10_abs_iqr = p75 - p25;
    out.abs_p50 = std::pow(10.0, p50);
    out.abs_p90 = std::pow(10.0, p90);
    out.abs_p99 = std::pow(10.0, p99);
  }

  if (!acc.tensor_rms.empty()) {
    const double mean = std::accumulate(acc.tensor_rms.begin(), acc.tensor_rms.end(), 0.0) /
                        static_cast<double>(acc.tensor_rms.size());
    double var = 0.0;
    for (const double v : acc.tensor_rms) {
      const double d = v - mean;
      var += d * d;
    }
    var /= static_cast<double>(acc.tensor_rms.size());

    out.tensor_rms_mean = mean;
    out.tensor_rms_std = std::sqrt(std::max(0.0, var));
    out.tensor_rms_cv = safe_ratio(out.tensor_rms_std, std::abs(mean) + kNumericEpsilon);

    const auto minmax =
        std::minmax_element(acc.tensor_rms.begin(), acc.tensor_rms.end());
    out.tensor_rms_min = *minmax.first;
    out.tensor_rms_max = *minmax.second;
    out.tensor_rms_max_over_min =
        safe_ratio(out.tensor_rms_max, out.tensor_rms_min + kNumericEpsilon);
  }

  out.max_abs_tensor_name = acc.max_abs_tensor_name;
  out.max_abs_tensor_value = acc.max_abs_tensor_value;

  out.buffer_tensor_count = buffer_acc.buffer_tensor_count;
  out.total_buffer_count = buffer_acc.total_buffer_count;
  out.finite_buffer_count = buffer_acc.finite_buffer_count;
  out.nan_buffer_count = buffer_acc.nan_buffer_count;
  out.inf_buffer_count = buffer_acc.inf_buffer_count;
  out.finite_buffer_ratio = safe_ratio(
      static_cast<double>(out.finite_buffer_count),
      static_cast<double>(out.total_buffer_count));
  out.max_abs_buffer_name = buffer_acc.max_abs_buffer_name;
  out.max_abs_buffer_value = buffer_acc.max_abs_buffer_value;

  out.matrix_tensor_count = spectral_acc.matrix_tensor_count;
  out.spectral_tensor_count = spectral_acc.spectral_tensor_count;
  out.spectral_skipped_tensor_count = spectral_acc.spectral_skipped_tensor_count;
  out.spectral_failed_tensor_count = spectral_acc.spectral_failed_tensor_count;
  out.spectral_norm_mean = safe_ratio(
      spectral_acc.sum_spectral_norm,
      static_cast<double>(out.spectral_tensor_count));
  out.spectral_norm_max = spectral_acc.max_spectral_norm;
  out.stable_rank_mean = safe_ratio(
      spectral_acc.sum_stable_rank,
      static_cast<double>(out.spectral_tensor_count));
  out.stable_rank_min = (out.spectral_tensor_count > 0)
                            ? spectral_acc.min_stable_rank
                            : 0.0;
  out.effective_rank_mean = safe_ratio(
      spectral_acc.sum_effective_rank,
      static_cast<double>(out.spectral_tensor_count));
  out.effective_rank_min = (out.spectral_tensor_count > 0)
                               ? spectral_acc.min_effective_rank
                               : 0.0;
  out.row_norm_cv_mean = safe_ratio(
      spectral_acc.sum_row_norm_cv,
      static_cast<double>(out.spectral_tensor_count));
  out.col_norm_cv_mean = safe_ratio(
      spectral_acc.sum_col_norm_cv,
      static_cast<double>(out.spectral_tensor_count));

  std::vector<double> effective_ranks{};
  effective_ranks.reserve(low_effective_rank_candidates.size());
  double inv_rank_sum = 0.0;
  for (const auto& item : low_effective_rank_candidates) {
    const double rank = std::max(0.0, item.value);
    effective_ranks.push_back(rank);
    inv_rank_sum += 1.0 / (rank + kNumericEpsilon);
  }
  out.network_capacity_tensor_count =
      static_cast<std::uint64_t>(effective_ranks.size());
  if (!effective_ranks.empty() && inv_rank_sum > 0.0) {
    out.network_global_entropic_capacity =
        static_cast<double>(effective_ranks.size()) / inv_rank_sum;
    out.network_entropic_bottleneck_min =
        *std::min_element(effective_ranks.begin(), effective_ranks.end());
    out.network_effective_rank_p50 = sorted_quantile_(effective_ranks, 0.50);
    out.network_effective_rank_p90 = sorted_quantile_(effective_ranks, 0.90);
  }

  const std::size_t top_k = static_cast<std::size_t>(effective.anomaly_top_k);
  out.top_nonfinite_ratio_tensors =
      take_top_k_(std::move(nonfinite_candidates), top_k, /*descending=*/true);
  out.top_max_abs_over_rms_tensors =
      take_top_k_(std::move(max_abs_over_rms_candidates), top_k, /*descending=*/true);
  out.top_near_zero_ratio_tensors =
      take_top_k_(std::move(near_zero_candidates), top_k, /*descending=*/true);
  out.top_low_stable_rank_tensors =
      take_top_k_(std::move(low_stable_rank_candidates), top_k, /*descending=*/false);
  out.top_low_effective_rank_tensors =
      take_top_k_(std::move(low_effective_rank_candidates), top_k, /*descending=*/false);
  out.top_high_spectral_norm_tensors =
      take_top_k_(std::move(high_spectral_norm_candidates), top_k, /*descending=*/true);

  return out;
}

